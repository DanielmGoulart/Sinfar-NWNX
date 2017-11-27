#include "C2da.h"
#include <memory>

#ifdef __linux__
#define _stricmp strcasecmp
#endif

C2da::C2da(void)
{
}

bool strisblank(char* str)
{
	int pos=0;
	while (str[pos] != 0)
	{
		if (!isspace(str[pos])) return false;
		pos++;
	}
	return true;
}

size_t C2da::GetColumnIndexByName(const std::string& column_name)
{
	for (size_t col_index = 0; col_index<columns_data.size(); col_index++)
	{
		if (_stricmp(columns_header.at(col_index).c_str(), column_name.c_str())==0) return col_index;
	}
	return -1;
}

std::string C2da::GetColumnNameByIndex(size_t column_index)
{
	if (column_index < columns_header.size())
	{
		return columns_header.at(column_index);
	}
	return "";
}

vector<string>* C2da::GetColumnByName(string column_name)
{
	size_t column_index = GetColumnIndexByName(column_name);
	if (column_index < columns_data.size())
	{
		return &(columns_data.at(column_index));
	}
	return NULL;
}

size_t C2da::GetRowCount()
{
	size_t rows_count = 0;
	for (size_t no_col=0; no_col<columns_data.size(); no_col++)
	{
		if (columns_data.at(no_col).size() > rows_count)
		{
			rows_count = columns_data.at(no_col).size();
		}
	}
	return rows_count;
}

size_t C2da::GetColCount()
{
	return columns_data.size();
}

void C2da::SetLastRow(size_t row)
{
	for (size_t no_col=0; no_col<columns_data.size(); no_col++)
	{
		columns_data.at(no_col).resize(row+1);
	}
}

void C2da::LoadFromFile(ifstream& f)
{
	char line[10000];
	//Line 1 = file format version
	f.getline(line, 10000);
	if (strstr((const char*)line, "2DA V2.0") == NULL) return;

	//Line 2 = blank or optional default
	f.getline(line, 10000);

	//Find the next not blank line witch is the columns header
	do
	{
		f.getline(line, 10000);
	}
	while (strisblank(line) && !f.eof());

	//Add the columns name to the map
	bool in_a_word=false;
	string word;
	int pos=0;
	while (true)
	{
		if (isspace(line[pos]) || line[pos]==0)
		{
			if (in_a_word)
			{
				columns_header.push_back(word);
				vector<string> v_str;
				columns_data.push_back(v_str);
				in_a_word = false;
			}
		}
		else
		{
			if (in_a_word)
			{
				word += line[pos];
			}
			else
			{
				word = line[pos];
				in_a_word = true;
			}
		}
		if (line[pos] == 0) break;
		pos++;
	}

	//Read all rows
	unsigned int no_col;
	bool is_row_index;
	while (!f.eof())
	{
		line[0] = 0;
		f.getline(line, 10000);
		if (strisblank(line)) continue;
		//Read all columns in the row
		pos=0;
		no_col=0;
		is_row_index=true;
		while (no_col < columns_header.size())
		{
			if (isspace(line[pos]) || line[pos]==0)
			{
				if (in_a_word)
				{
					if (is_row_index)
					{
						is_row_index = false;
					}
					else
					{
						columns_data.at(no_col).push_back(word);
						no_col++;
					}
					in_a_word = false;
				}
			}
			else
			{
				if (in_a_word)
				{
					word += line[pos];
				}
				else
				{
					if (line[pos] == '"')
					{
						word = "\"";
						pos++;
						while (line[pos] != 0)
						{
							if (line[pos] == '"')
							{
								word += "\"";
								break;
							}
							else
							{
								word += line[pos];
							}
							pos++;
						}
					}
					else
					{
						word = line[pos];
					}
					in_a_word = true;
				}
			}
			if (line[pos] == 0) break;
			pos++;
		}
	}
	f.close();
}

C2da::C2da(const std::string& filename)
{
	ifstream f(filename);
	if (!f) return;
	LoadFromFile(f);
}

#ifdef WIN32
C2da::C2da(const wchar_t* filename)
{
	ifstream f(filename);
	if (!f) return;
	LoadFromFile(f);
}
#endif

C2da::~C2da(void)
{
}

std::string C2da::GetString(size_t column_index, size_t row)
{
	if (column_index < columns_data.size())
	{
		vector<string>& column_data = columns_data.at(column_index);
		if (row < column_data.size())
		{
			string str = column_data.at(row);
			if (str != "****" && str != "")
			{
				if (str.at(0) == '"')
				{
					return str.substr(1, str.length()-2);
				}
				else
				{
					return str;
				}
			}
		}
	}
	return "";
}

std::string C2da::GetString(const std::string& column_name, size_t row)
{
	return GetString(GetColumnIndexByName(column_name), row);
}

int C2da::GetInt(const std::string& column_name, size_t row)
{
	std::string value = GetString(GetColumnIndexByName(column_name), row);
	char* end;
	if (value.length() > 2 && value.substr(0, 2) == "0x")
	{
		return std::strtol(value.c_str(), &end, 16);
	}
	else
	{
		return std::strtol(value.c_str(), &end, 10);
	}
}

float C2da::GetFloat(const std::string& column_name, size_t row)
{
	return atof(GetString(column_name, row).c_str());
}

char* C2da::GetString(const std::string& column_name, size_t row, char* buffer, size_t size)
{
	std::string value = GetString(GetColumnIndexByName(column_name), row);
	if (!value.empty())
	{
		size--;
		strncpy(buffer, value.c_str(), size);
		buffer[size] = 0;
		return buffer;
	}
	return NULL;
}

void C2da::SetString(const std::string& column_name, size_t row, const std::string& value)
{
	std::unique_ptr<char> new_value_ptr((char*)malloc(value.length()+3/*quotes + ending 0*/));
	char* temp = new_value_ptr.get()+1;
	bool space_found = false;;
	for (const char* value_c_str=value.c_str(); *value_c_str; value_c_str++)
	{
		switch (*value_c_str)
		{
			case '"':
			case '\n':
			case '\r':
			case '\v':
			case '\f':
				continue;
			case ' ':
			case '\t':
				space_found = true;
			default:
				*temp = *value_c_str;
				temp++;
		}
	}
	char* new_value_c_str = new_value_ptr.get()+1;
	if (space_found)
	{
		new_value_ptr.get()[0] = '"';
		*temp = '"';
		temp++;
		new_value_c_str--;
	}
	std::string new_value;
	if (temp == new_value_c_str)
	{
		new_value = "****";
	}
	else
	{
		*temp = 0;
		new_value = new_value_c_str;
	}
	
	vector<string>* p_column = GetColumnByName(column_name);
	if (p_column == NULL)
	{
		columns_header.push_back(column_name);
		vector<string> column;
		columns_data.push_back(column);
		p_column = &(columns_data.at(columns_data.size()-1));
	}

	if (row >= p_column->size())
	{
		for (unsigned int last_row=p_column->size(); last_row<row; last_row++)
		{
			p_column->push_back("****");
		}
		p_column->push_back(new_value);
	}
	else
	{
		(*p_column)[row] = new_value;
	}
}

void C2da::SaveToFile(const std::string& filename)
{
	ofstream f(filename);
	if (!f) return;
	f << "2DA V2.0";
	f << endl;
	f << endl;

	size_t* columns_length = new size_t[columns_data.size()];
	for (size_t no_col=0; no_col<columns_header.size(); no_col++)
	{
		columns_length[no_col] = columns_header.at(no_col).size();
		for (size_t no_row=0; no_row<columns_data.at(no_col).size(); no_row++)
		{
			if (columns_length[no_col] < columns_data.at(no_col).at(no_row).size())
			{
				columns_length[no_col] = columns_data.at(no_col).at(no_row).size();
			}
		}
	}
	f << "\t\t";
	for (size_t no_col=0; no_col<columns_header.size(); no_col++)
	{
		string column_name = columns_header.at(no_col);
		f << column_name.append(columns_length[no_col]-column_name.size(), ' ');
		f << "\t";
	}
	f << endl;

	size_t rows_count = GetRowCount();
	for (size_t no_row=0; no_row<rows_count; no_row++)
	{
		f << no_row;
		f << "\t\t";
		for (size_t no_col=0; no_col<columns_header.size(); no_col++)
		{
			string value;
			if (no_row >= columns_data.at(no_col).size())
				value = "****";
			else
			{
				value = columns_data.at(no_col).at(no_row);
				/*if (value.find(" ") != string::npos)
				{
					value.insert(0, "\"");
					value.insert(value.size(), "\"");
					//value = string("\"") + value + string("\"");
				}*/
				if (value == "")
				{
					value = "****";
				}
			}
			f << value.append(columns_length[no_col]-value.size(), ' ');
			f << "\t";
		}
		f << endl;
	}

	delete[] columns_length;
	f.close();

}