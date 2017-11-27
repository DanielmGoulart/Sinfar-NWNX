	CExoString table_name = vm_pop_string();
	CExoString col_name = vm_pop_string();
	uint32_t start_row_index = vm_pop_int();
	CExoString exo_prefix = vm_pop_string();
	const char* prefix = (exo_prefix.text?exo_prefix.text:"");
	int result = -1;
	C2da* table = get_cached_2da(table_name);
	if (table)
	{
		size_t col_index = table->GetColumnIndexByName(col_name);
		if (col_index != 0xFFFFFFFF)
		{
			size_t row_count = table->GetRowCount();
			if (start_row_index >= row_count || start_row_index < 0) start_row_index = 0;
			for (size_t row_index=start_row_index; row_index<row_count; row_index++)
			{
				if (VM_FUNC_FIND_NEXT_2DA_ROW_COMPARATOR)
				{
					result =  row_index;
					break;
				}
			}
			if (result == -1)
			{
				for (size_t row_index=0; row_index<start_row_index; row_index++)
				{
					if (VM_FUNC_FIND_NEXT_2DA_ROW_COMPARATOR)
					{
						result = row_index;
						break;
					}
				}
			}
		}
	}
	vm_push_int(result);
