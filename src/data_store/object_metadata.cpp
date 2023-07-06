#include "object_metadata.hpp"

#include <algorithm>
#include <stdexcept>

namespace dyad {
namespace data_store {
    
ObjectSegmentID::ObjectSegmentID(ObjectSegmentID::entry_list::iterator& it,
    const ObjectMetadata& mdata)
    : m_segment_it(it)
    , m_locs_ref(mdata.m_segment_locs)
{}

std::size_t ObjectSegmentID::get_segment_idx() const
{
    return static_cast<std::size_t>(
        std::distance<ObjectSegmentID::entry_list::const_iterator>(
            m_locs_ref.begin(),
            m_segment_it
        )
    );
}
    
ObjectMetadata::ObjectMetadata(const std::string& object_name)
    : m_name(object_name)
    , m_object_size(0)
{}


} /* namespace data_store */
} /* namespace dyad */