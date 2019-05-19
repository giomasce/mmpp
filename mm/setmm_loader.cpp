
#include "setmm_loader.h"

#include <giolib/proc_stats.h>

#include "mm/reader.h"
#include "utils/utils.h"

SetMmImpl::SetMmImpl(const boost::filesystem::path &filename, const boost::filesystem::path &cache_filename)
{
    std::cout << "Reading database from file " << filename << " using cache in file " << cache_filename << std::endl;
    TextProgressBar tpb;
    FileTokenizer ft(filename, &tpb);
    Reader p(ft, false, true);
    p.run();
    tpb.finished();
    this->lib = new LibraryImpl(p.get_library());
    std::shared_ptr< ToolboxCache > cache = std::make_shared< FileToolboxCache >(cache_filename);
    std::cout << "Memory usage after loading the library: " << size_to_string(gio::get_used_memory()) << std::endl;
    this->tb = new LibraryToolbox(*this->lib, "|-", cache);
    std::cout << "Memory usage after creating the toolbox: " << size_to_string(gio::get_used_memory()) << std::endl;
    std::cout << "The library has " << this->lib->get_symbols_num() << " symbols and " << this->lib->get_labels_num() << " labels" << std::endl << std::endl;
}

SetMmImpl::~SetMmImpl() {
    delete this->lib;
    delete this->tb;
}

SetMm::SetMm(const boost::filesystem::path &filename, const boost::filesystem::path &cache_filename) :
    inner(filename, cache_filename), lib(*inner.lib), tb(*inner.tb)
{
}

const SetMm &get_set_mm() {
    //std::cout << "Base resource directory is: " << platform_get_resources_base() << std::endl;
    static SetMm data(platform_get_resources_base() / "set.mm", platform_get_resources_base() / "set.mm.cache");
    return data;
}
