#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <stack>
#include <sstream>


namespace olc::utils {
	class DataFile {
		public:
			void set_string(const std::string &s_string, const size_t n_item = 0);
			const std::string get_string(const size_t n_item = 0) const;
			void set_real(const double d, const size_t n_item = 0);
			double get_real(const size_t n_item = 0) const;
			void set_int(const int i, const size_t n_item = 0);
			int get_int(const size_t n_item = 0) const;
			size_t get_value_count() const;
			DataFile &operator[](const std::string &name) {
				// check if map contains name, else add it
				if (!m_mapObjects.contains(name)) {
					m_mapObjects[name] = m_vecObjects.size();
					// NOTE huh, why constructor here?
					m_vecObjects.push_back({name, DataFile()});
				}
				// use the u_map to index the vector and return the second pair entry
				return m_vecObjects[m_mapObjects[name]].second;
			}
		private:
			// list of strings that make up a propery value
			std::vector<std::string>											m_vContent;
			// vector holds the order
			std::vector<std::pair<std::string, DataFile>> m_vecObjects;
			// this maps the name of the child node to an index
			std::unordered_map<std::string, size_t>				m_mapObjects;
	};
} // namespace olc::utils
