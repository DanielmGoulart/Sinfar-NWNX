#ifndef C2DA_H_
#define C2DA_H_

#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <fstream>

using namespace std;

class C2da
{
private:
	vector<string> columns_header;
	vector<vector<string>> columns_data;

	void LoadFromFile(ifstream& f);
public:
	C2da(void);
	C2da(const std::string& filename);
#ifdef WIN32
	C2da(const wchar_t* filename);
#endif
	~C2da(void);
	

	vector<string>* GetColumnByName(string column_name);

	size_t GetRowCount();
	size_t GetColCount();

	size_t GetColumnIndexByName(const std::string& column_name);
	std::string GetColumnNameByIndex(size_t column_index);

	std::string GetString(size_t column_index, size_t row);
	std::string GetString(const std::string& column_name, size_t row);
	int GetInt(const std::string& column_name, size_t row);
	float GetFloat(const std::string& column_name, size_t row);
	char* GetString(const std::string& column_name, size_t row, char* buffer, size_t size);
	void SetString(const std::string& column_name, size_t row, const std::string& value);

	void SetLastRow(size_t row);

	void SaveToFile(const std::string& filename);
};


#endif
