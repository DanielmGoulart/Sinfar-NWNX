#include <boost/test/unit_test.hpp>
#include <Windows.h>
#include <nwncx/sinfar/nwncx_sinfar.h>
#include <nwncx/sinfar/resman.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace nwncx;
using namespace nwncx::sinfar;

CExoString* __fastcall CExoStringSetText_Test(CExoString* exo_str, int edx, const char* new_str)
{
	if (exo_str->text) free(exo_str->text);
	if (new_str)
	{
		exo_str->text = _strdup(new_str);
		exo_str->len = strlen(new_str);
	}
	else
	{
		exo_str->text = NULL;
		exo_str->len = 0;
	}
	return exo_str;
}
void __fastcall DestroyCExoString_Test(CExoString* exo_str)
{
	if (exo_str->text) free(exo_str->text);
}

CExoString* __fastcall resolve_filename_org_test(void*, int edx, CExoString* result, CExoString* source, uint16_t type)
{
	CExoStringSetText_Test(result, 0, source->text);
	return result;
}

void add_single_erf_file(const std::string& erf_filename)
{
	CExoString erf_exo_str(erf_filename.c_str());
	add_encapsulated_resource_file_hook(NULL, 0, &erf_exo_str);
}
void remove_single_erf_file(const std::string& erf_filename)
{
	CExoString erf_exo_str(erf_filename.c_str());
	remove_encapsulated_resource_file_hook(NULL, 0, &erf_exo_str);
}
void add_single_directory(const std::string& dir_path)
{
	CExoString dir_exo_str(dir_path.c_str());
	add_resource_directory_hook(NULL, 0, &dir_exo_str);
}
void remove_single_directory(const std::string& dir_path)
{
	CExoString dir_exo_str(dir_path.c_str());
	remove_resource_directory_hook(NULL, 0, &dir_exo_str);
}

struct ResmanTestFixture
{
     ResmanTestFixture() : exo_base(new CExoBase){
		CExoStringSetText = CExoStringSetText_Test;
		DestroyCExoString = DestroyCExoString_Test;
		resolve_filename_org = resolve_filename_org_test;
		p_exo_base = &exo_base;
		SetCurrentDirectoryA("Z:/Windows8.1/NWN");
	 }
     ~ResmanTestFixture() { 
		delete exo_base;
	 }
	 CExoBase* exo_base;
};

BOOST_FIXTURE_TEST_CASE(ResmanTest_AddAndRemoveContainers, ResmanTestFixture)
{	
	add_single_erf_file("hak/sinfar_top_25");
	add_single_erf_file("hak/sinfar_top_25");
	BOOST_CHECK_EQUAL(105, best_resources.size());
	remove_single_erf_file("hak/sinfar_top_25");
	BOOST_CHECK_EQUAL(0, best_resources.size());
	remove_single_erf_file("hak/sinfar_top_25");
	BOOST_CHECK_EQUAL(0, best_resources.size());

	add_single_directory("./override");
	add_single_directory("override");
	BOOST_CHECK_EQUAL(6, best_resources.size());
	remove_single_directory("./override");
	BOOST_CHECK_EQUAL(6, best_resources.size());
	remove_single_directory("override");
	BOOST_CHECK_EQUAL(0, best_resources.size());
}