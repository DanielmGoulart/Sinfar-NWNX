#include "nwscript_funcs.h"
#include <GFF/quick_read_gff.h>
#include "../erf.h"

namespace
{
	
VM_FUNC_NEW(QuickReadGFF, 172)
{
	CExoString filename = vm_pop_string();
	CExoString field_name = vm_pop_string();
	CExoString result;
	if (erf::get_current_script_in_core())
	{
		result = quick_read_gff(filename, field_name).c_str();
	}
	vm_push_string(&result);
}	

}