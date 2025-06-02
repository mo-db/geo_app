#include "save_sys.h"

namespace olc::utils {
void DataFile::set_string(const std::string &s_string, const size_t n_item) {
	if (n_item >= m_vContent.size()) { // NOTE why even resize the vector?
		m_vContent.resize(n_item + 1);
	}
	m_vContent[n_item] = s_string; // NOTE why doesn't push back the string??
}
const std::string DataFile::get_string(const size_t n_item) const {
	if (n_item >= m_vContent.size()) { // why even resize the vector?
		return "";
	}
	return m_vContent[n_item];
}
void DataFile::set_real(const double d, const size_t n_item) {
	set_string(std::to_string(d), n_item);
}
double DataFile::get_real(const size_t n_item) const {
	return std::atof(get_string(n_item).c_str()); // NOTE why c_str??
}
void DataFile::set_int(const int i, const size_t n_item) {
	set_string(std::to_string(i), n_item);
}
int DataFile::get_int(const size_t n_item) const {
	return std::atoi(get_string(n_item).c_str()); // NOTE why c_str??
}
size_t DataFile::get_value_count() const {
	return m_vContent.size();
}
} // namespace olc::utils
