/* Copyright (c) 2013-2016, EPFL/Blue Brain Project
 *                          Daniel.Nachbaur@epfl.ch
 *
 * This file is part of Brion <https://github.com/BlueBrain/Brion>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "synapses.h"

#include "circuit.h"
#include "synapsesIterator.h"
#include "synapsesStream.h"
#include "synapse.h"

#include "detail/circuit.h"
#include "detail/synapsesStream.h"

#include <brion/synapse.h>
#include <brion/synapseSummary.h>

#include <lunchbox/log.h>
#include <lunchbox/scopedMutex.h>

namespace brain
{

namespace
{
template<typename T> void _allocate( T& data, const size_t size )
{
    if( data )
        return;

    void* ptr;
    if( posix_memalign( &ptr, 32, size * sizeof(typename T::element_type) ))
    {
        LBWARN << "Memory alignment failed. Trying normal allocation"
               << std::endl;
        ptr = calloc( size, sizeof(typename T::element_type));
        if( !ptr )
            LBTHROW( std::bad_alloc( ));
    }
    data.reset((typename T::element_type*) ptr );
    // cppcheck-suppress memleak
}
}

struct Synapses::Impl
{
    Impl( const Circuit& circuit, const GIDSet& gids, const GIDSet& filterGIDs,
          const bool afferent, const SynapsePrefetch prefetch )
        : _circuit( circuit )
        , _gids( prefetch != SYNAPSEPREFETCH_ALL ? gids : GIDSet( ))
        , _filterGIDs( prefetch != SYNAPSEPREFETCH_ALL ? filterGIDs : GIDSet( ))
        , _afferent( afferent )
        , _size( 0 )
    {
        if( prefetch == SYNAPSEPREFETCH_NONE )
        {
            _loadConnectivity( gids, filterGIDs );
            return;
        }

        if( prefetch & SYNAPSEPREFETCH_ATTRIBUTES )
            _loadAttributes( gids, filterGIDs );
        if( prefetch & SYNAPSEPREFETCH_POSITIONS )
            _loadPositions( gids, filterGIDs );
    }

#define FILTER( gid )\
    if( !filterGIDs.empty() && filterGIDs.find( gid ) == filterGIDs.end( ))\
        continue;

    void _loadConnectivity( const GIDSet& gids,
                            const GIDSet& filterGIDs ) const
    {
        const brion::SynapseSummary& synapseSummary =
                _circuit._impl->getSynapseSummary();

        uint32_ts pres, posts;
        for( const auto gid : gids )
        {
            const auto& summary = synapseSummary.read( gid );

            for( size_t i = 0; i < summary.shape()[0]; ++i )
            {
                const uint32_t peerGid = summary[i][0];
                FILTER( peerGid );

                for( size_t j = 0; j < summary[i][_afferent ? 2 : 1]; ++j )
                {
                    pres.push_back( peerGid );
                    posts.push_back( gid );
                }
            }
        }

        _size = pres.size();
        _allocate( _preGID, _size );
        _allocate( _postGID, _size );
        memcpy( _preGID.get(), pres.data(), _size * sizeof(uint32_t));
        memcpy( _postGID.get(), posts.data(), _size * sizeof(uint32_t));

        if( !_afferent )
            _preGID.swap( _postGID );
    }

    void _loadAttributes( const GIDSet& gids, const GIDSet& filterGIDs ) const
    {
        if( _efficacy )
            return;

        const brion::Synapse& synapseAttributes =
                _circuit._impl->getSynapseAttributes( _afferent );
        const brion::Synapse* synapseExtra = _circuit._impl->getSynapseExtra();

        const bool haveExtra = _afferent && synapseExtra;
        const bool haveSize = _size > 0;
        _allocateAttributes( haveSize ? _size
                                     : synapseAttributes.getNumSynapses( gids ),
                             haveExtra );

        size_t i = 0;
        for( const auto gid : gids )
        {
            const auto& attr = synapseAttributes.read( gid,
                                                brion::SYNAPSE_ALL_ATTRIBUTES );
            const auto extra = haveExtra ? synapseExtra->read( gid, 1 )
                                         : brion::SynapseMatrix();
            for( size_t j = 0; j < attr.shape()[0]; ++j )
            {
                const uint32_t preGid = attr[j][0];
                FILTER( preGid );

                if( !haveSize )
                {
                    _preGID.get()[i] = preGid;
                    _postGID.get()[i] = gid;
                }

                if( haveExtra )
                    _index.get()[i] = extra[j][0];

                _delay.get()[i] = attr[j][1];
                _postSectionID.get()[i] = attr[j][2];
                _postSegmentID.get()[i] = attr[j][3];
                _postDistance.get()[i] = attr[j][4];
                _preSectionID.get()[i] = attr[j][5];
                _preSegmentID.get()[i] = attr[j][6];
                _preDistance.get()[i] = attr[j][7];
                _conductance.get()[i] = attr[j][8];
                _utilization.get()[i] = attr[j][9];
                _depression.get()[i] = attr[j][10];
                _facilitation.get()[i] = attr[j][11];
                _decay.get()[i] = attr[j][12];
                _efficacy.get()[i] = attr[j][17];
                ++i;
            }
        }

        if( !haveSize )
        {
            if( !_afferent )
                _preGID.swap( _postGID );
            _size = i;
        }
    }

    void _loadPositions( const GIDSet& gids, const GIDSet& filterGIDs ) const
    {
        if( _preSurfacePositionX )
            return;

        const bool haveSize = _size > 0;
        const brion::Synapse& synapsePositions =
                _circuit._impl->getSynapsePositions( _afferent );
        _allocatePositions( haveSize ? _size
                                     : synapsePositions.getNumSynapses( gids ));

        size_t i = 0;
        for( const auto gid : gids )
        {
            const auto& pos = synapsePositions.read( gid,
                                                     brion::SYNAPSE_POSITION );

            for( size_t j = 0; j < pos.shape()[0]; ++j )
            {
                const uint32_t preGid = pos[j][0];
                FILTER( preGid );

                if( !haveSize )
                {
                    _preGID.get()[i] = preGid;
                    _postGID.get()[i] = gid;
                }

                _preSurfacePositionX.get()[i] = pos[j][1];
                _preSurfacePositionY.get()[i] = pos[j][2];
                _preSurfacePositionZ.get()[i] = pos[j][3];
                _postSurfacePositionX.get()[i] = pos[j][4];
                _postSurfacePositionY.get()[i] = pos[j][5];
                _postSurfacePositionZ.get()[i] = pos[j][6];
                _preCenterPositionX.get()[i] = pos[j][7];
                _preCenterPositionY.get()[i] = pos[j][8];
                _preCenterPositionZ.get()[i] = pos[j][9];
                _postCenterPositionX.get()[i] = pos[j][10];
                _postCenterPositionY.get()[i] = pos[j][11];
                _postCenterPositionZ.get()[i] = pos[j][12];
                ++i;
            }
        }

        if( !haveSize )
        {
            if( !_afferent )
                _preGID.swap( _postGID );
            _size = i;
        }
    }

    void _allocateAttributes( const size_t size,
                              const bool allocateIndex ) const
    {
        if( allocateIndex )
            _allocate( _index, size );

        _allocate( _preGID, size );
        _allocate( _preSectionID, size );
        _allocate( _preSegmentID, size );
        _allocate( _preDistance, size );

        _allocate( _postGID, size );
        _allocate( _postSectionID, size );
        _allocate( _postSegmentID, size );
        _allocate( _postDistance, size );

        _allocate( _delay, size );
        _allocate( _conductance, size );
        _allocate( _utilization, size );
        _allocate( _depression, size );
        _allocate( _facilitation, size );
        _allocate( _decay, size );
        _allocate( _efficacy, size );
    }

    void _allocatePositions( const size_t size ) const
    {
        _allocate( _preGID, size );
        _allocate( _preSurfacePositionX, size );
        _allocate( _preSurfacePositionY, size );
        _allocate( _preSurfacePositionZ, size );
        _allocate( _preCenterPositionX, size );
        _allocate( _preCenterPositionY, size );
        _allocate( _preCenterPositionZ, size );

        _allocate( _postGID, size );
        _allocate( _postSurfacePositionX, size );
        _allocate( _postSurfacePositionY, size );
        _allocate( _postSurfacePositionZ, size );
        _allocate( _postCenterPositionX, size );
        _allocate( _postCenterPositionY, size );
        _allocate( _postCenterPositionZ, size );
    }

    void _ensureAttributes() const
    {
        if( _hasAttributes( ))
            return;

        lunchbox::ScopedWrite mutex( _lock );
        _loadAttributes( _gids, _filterGIDs );
    }

    void _ensurePositions() const
    {
        if( _hasPositions( ))
            return;

        lunchbox::ScopedWrite mutex( _lock );
        _loadPositions( _gids, _filterGIDs );
    }

    bool _hasAttributes() const
    {
        lunchbox::ScopedRead mutex( _lock );
        return _efficacy.get() != nullptr;
    }

    bool _hasPositions() const
    {
        lunchbox::ScopedRead mutex( _lock );
        return _preSurfacePositionX.get() != nullptr;
    }

    const Circuit& _circuit;
    const GIDSet _gids;
    const GIDSet _filterGIDs;
    const bool _afferent;

    template<typename T>
    struct FreeDeleter
    {
        void operator()( T* ptr ) { free( ptr ); }
    };

    typedef std::unique_ptr< uint32_t, FreeDeleter<uint32_t> > UIntPtr;
    typedef std::unique_ptr< int, FreeDeleter<int> > IntPtr;
    typedef std::unique_ptr< size_t, FreeDeleter<size_t> > size_tPtr;
    typedef std::unique_ptr< float, FreeDeleter<float> > floatPtr;

    mutable size_t _size;

    mutable size_tPtr _index;

    mutable UIntPtr _preGID;
    mutable UIntPtr _preSectionID;
    mutable UIntPtr _preSegmentID;
    mutable floatPtr _preDistance;
    mutable floatPtr _preSurfacePositionX;
    mutable floatPtr _preSurfacePositionY;
    mutable floatPtr _preSurfacePositionZ;
    mutable floatPtr _preCenterPositionX;
    mutable floatPtr _preCenterPositionY;
    mutable floatPtr _preCenterPositionZ;

    mutable UIntPtr _postGID;
    mutable UIntPtr _postSectionID;
    mutable UIntPtr _postSegmentID;
    mutable floatPtr _postDistance;
    mutable floatPtr _postSurfacePositionX;
    mutable floatPtr _postSurfacePositionY;
    mutable floatPtr _postSurfacePositionZ;
    mutable floatPtr _postCenterPositionX;
    mutable floatPtr _postCenterPositionY;
    mutable floatPtr _postCenterPositionZ;

    mutable floatPtr _delay;
    mutable floatPtr _conductance;
    mutable floatPtr _utilization;
    mutable floatPtr _depression;
    mutable floatPtr _facilitation;
    mutable floatPtr _decay;
    mutable IntPtr _efficacy;

    mutable lunchbox::Lock _lock;
};

Synapses::Synapses( const Circuit& circuit, const GIDSet& pre,
                    const GIDSet& post, const bool afferent,
                    const SynapsePrefetch prefetch )
    : _impl( new Impl( circuit, pre, post, afferent, prefetch ))
{
}

Synapses::~Synapses()
{
}

Synapses::Synapses( const SynapsesStream& stream )
    : _impl( new Impl( stream._impl->_circuit, stream._impl->_gids,
                       stream._impl->_filterGIDs, stream._impl->_afferent,
                       stream._impl->_prefetch ))
{
}

Synapses::Synapses( const Synapses& rhs )
    : _impl( rhs._impl )
{
}

Synapses::Synapses( Synapses&& rhs )
    : _impl( std::move( rhs._impl ))
{
}

Synapses& Synapses::operator=( const Synapses& rhs )
{
    if( this != &rhs )
        _impl = rhs._impl;
    return *this;
}

Synapses& Synapses::operator=( Synapses&& rhs )
{
    if( this != &rhs )
        _impl = std::move( rhs._impl );
    return *this;
}

size_t Synapses::size() const
{
    lunchbox::ScopedRead mutex( _impl->_lock );
    return _impl->_size;
}

bool Synapses::empty() const
{
    return size() == 0;
}

Synapses::const_iterator Synapses::begin() const
{
    return const_iterator( *this, 0 );
}

Synapses::const_iterator Synapses::end() const
{
    return const_iterator( *this, size( ));
}

Synapse Synapses::operator[]( const size_t index_ ) const
{
    return Synapse( *this, index_ );
}

const size_t* Synapses::indices() const
{
    lunchbox::ScopedRead mutex( _impl->_lock );
    if( !_impl->_index )
        LBTHROW( std::runtime_error( "No synapse index file available" ));
    return _impl->_index.get();
}

const uint32_t* Synapses::preGIDs() const
{
    return _impl->_preGID.get();
}

const uint32_t* Synapses::preSectionIDs() const
{
    _impl->_ensureAttributes();
    return _impl->_preSectionID.get();
}

const uint32_t* Synapses::preSegmentIDs() const
{
    _impl->_ensureAttributes();
    return _impl->_preSegmentID.get();
}

const float* Synapses::preDistances() const
{
    _impl->_ensureAttributes();
    return _impl->_preDistance.get();
}

const float* Synapses::preSurfaceXPositions() const
{
    _impl->_ensurePositions();
    return _impl->_preSurfacePositionX.get();
}

const float* Synapses::preSurfaceYPositions() const
{
    _impl->_ensurePositions();
    return _impl->_preSurfacePositionY.get();
}

const float* Synapses::preSurfaceZPositions() const
{
    _impl->_ensurePositions();
    return _impl->_preSurfacePositionZ.get();
}

const float* Synapses::preCenterXPositions() const
{
    _impl->_ensurePositions();
    return _impl->_preCenterPositionX.get();
}

const float* Synapses::preCenterYPositions() const
{
    _impl->_ensurePositions();
    return _impl->_preCenterPositionY.get();
}

const float* Synapses::preCenterZPositions() const
{
    _impl->_ensurePositions();
    return _impl->_preCenterPositionZ.get();
}

const uint32_t* Synapses::postGIDs() const
{
    return _impl->_postGID.get();
}

const uint32_t* Synapses::postSectionIDs() const
{
    _impl->_ensureAttributes();
    return _impl->_postSectionID.get();
}

const uint32_t* Synapses::postSegmentIDs() const
{
    _impl->_ensureAttributes();
    return _impl->_postSegmentID.get();
}

const float* Synapses::postDistances() const
{
    _impl->_ensureAttributes();
    return _impl->_postDistance.get();
}

const float* Synapses::postSurfaceXPositions() const
{
    _impl->_ensurePositions();
    return _impl->_postSurfacePositionX.get();
}

const float* Synapses::postSurfaceYPositions() const
{
    _impl->_ensurePositions();
    return _impl->_postSurfacePositionY.get();
}

const float* Synapses::postSurfaceZPositions() const
{
    _impl->_ensurePositions();
    return _impl->_postSurfacePositionZ.get();
}

const float* Synapses::postCenterXPositions() const
{
    _impl->_ensurePositions();
    return _impl->_postCenterPositionX.get();
}

const float* Synapses::postCenterYPositions() const
{
    _impl->_ensurePositions();
    return _impl->_postCenterPositionY.get();
}

const float* Synapses::postCenterZPositions() const
{
    _impl->_ensurePositions();
    return _impl->_postCenterPositionZ.get();
}

const float* Synapses::delays() const
{
    _impl->_ensureAttributes();
    return _impl->_delay.get();
}

const float* Synapses::conductances() const
{
    _impl->_ensureAttributes();
    return _impl->_conductance.get();
}

const float* Synapses::utilizations() const
{
    _impl->_ensureAttributes();
    return _impl->_utilization.get();
}

const float* Synapses::depressions() const
{
    _impl->_ensureAttributes();
    return _impl->_depression.get();
}

const float* Synapses::facilitations() const
{
    _impl->_ensureAttributes();
    return _impl->_facilitation.get();
}

const float* Synapses::decays() const
{
    _impl->_ensureAttributes();
    return _impl->_decay.get();
}

const int* Synapses::efficacys() const
{
    _impl->_ensureAttributes();
    return _impl->_efficacy.get();
}

}
