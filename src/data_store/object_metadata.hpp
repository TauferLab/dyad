#ifndef DYAD_DATA_STORE_FILE_METADATA_HPP
#define DYAD_DATA_STORE_FILE_METADATA_HPP

#include <list>
#include <string>
#include <tuple>

namespace dyad {
namespace data_store {
    
class ObjectMetadata;
    
class ObjectSegmentID {

    private:
        
        friend ObjectMetadata;
        
        using entry_list = std::list<
            std::tuple<
                std::size_t,
                std::size_t,
                std::size_t,
                std::size_t,
                bool
            >
        >;
        
        ObjectSegmentID(entry_list::iterator& it, const ObjectMetadata& mdata);
        
        std::size_t get_block_idx() const;
        
        entry_list::iterator m_block_it;
        
        const entry_list& m_locs_ref;
    
};

class ObjectMetadata {
    
    public:
        
        ObjectMetadata(const std::string& object_name);


    private:
        
        friend ObjectSegmentID;

        std::string m_name;
        std::size_t m_object_size;
        std::list<
            std::tuple<
                std::size_t, // Device ID
                std::size_t, // Log offset
                std::size_t, // Segment size
                std::size_t, // Secondary IMDS ID
                bool // If true, blocks are in secondary IMDS
            >
        > m_segment_locs;

};

} /* namespace data_store */
} /* namespace dyad */

#endif /* DYAD_DATA_STORE_FILE_METADATA_HPP */