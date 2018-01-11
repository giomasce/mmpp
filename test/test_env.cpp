
#include "test/test_env.h"

#include "mm/reader.h"
#include "platform.h"
#include "utils/utils.h"

using namespace std;

TestEnvironmentInner::TestEnvironmentInner(const boost::filesystem::path &filename, const boost::filesystem::path &cache_filename)
{
    cout << "Reading database from file " << filename << " using cache in file " << cache_filename << endl;
    TextProgressBar tpb;
    FileTokenizer ft(filename, &tpb);
    Reader p(ft, false, true);
    p.run();
    tpb.finished();
    this->lib = new LibraryImpl(p.get_library());
    shared_ptr< ToolboxCache > cache = make_shared< FileToolboxCache >(cache_filename);
    cout << "Memory usage after loading the library: " << size_to_string(platform_get_current_rss()) << endl;
    this->tb = new LibraryToolbox(*this->lib, "|-", true, cache);
    cout << "Memory usage after creating the toolbox: " << size_to_string(platform_get_current_rss()) << endl;
    cout << "The library has " << this->lib->get_symbols_num() << " symbols and " << this->lib->get_labels_num() << " labels" << endl << endl;
}

TestEnvironmentInner::~TestEnvironmentInner() {
    delete this->lib;
    delete this->tb;
}

TestEnvironment::TestEnvironment(const boost::filesystem::path &filename, const boost::filesystem::path &cache_filename) :
    inner(filename, cache_filename), lib(*inner.lib), tb(*inner.tb)
{
}

const TestEnvironment &get_set_mm() {
    static TestEnvironment data(platform_get_resources_base() / "set.mm", platform_get_resources_base() / "set.mm.cache");
    return data;
}
