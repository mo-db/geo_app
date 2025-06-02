#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <stack>
#include <sstream>


namespace olc::utils {
	// recurcsive container
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
			static bool write( // NOTE why static here?
					const DataFile &n,
					const std::string &file_name,
					const std::string &s_indent = "\t",
					const char s_list_sep = ',') 
			{
				// Cache indentation level
				size_t n_indent_count = 0;
				// Cache sperator string for convenience
				std::string s_sep = std::string(1, s_list_sep) + " ";

				std::ofstream file(file_name); // NOTE wtf is this syntax

				// recursive lambda!?
				std::function<void(const DataFile &, std::ofstream &)> write = [&](
						const DataFile &n, std::ofstream &file) {

					// Lambda creates string given indentation preferences
					auto indent = [&](const std::string& s_string, const size_t n_count)
					{
						std::string s_out;
						for (size_t n = 0; n < n_count; n++) s_out += s_string;
						return s_out;
					};

					// iterate trough each property of this node
					for (auto const &property : n.m_vecObjects) {

						// does property contain sub objects?
						if (property.second.m_vecObjects.empty()) {

							// No, so it's an assigned field and should just be written. If the property
							// is flagged as comment, it has no assignment potential. First write the 
							// property name
							file << indent(s_indent, n_indent_count)<< property.first << " = ";

							// Second, write the property value (or values, seperated by provided
							// separation charater
							size_t n_items = property.second.get_value_count();
							for (size_t i = 0; i < property.second.get_value_count(); i++) {

								// If the Value being written, in string form, contains the separation
								// character, then the value must be written inside quotation marks. Note, 
								// that if the Value is the last of a list of Values for a property, it is
								// not suffixed with the separator
								size_t x = property.second.get_string(i).find_first_of(s_list_sep);
								if (x != std::string::npos) {
									// Value contains separator, so wrap in quotes
									file << "\"" << property.second.get_string(i) <<
										((n_items > 1) ? s_sep: "");
								} else {
									// Value does not contain separator, so just write out
									file << property.second.get_string() <<
										((n_items > 1) ? s_sep : "");
								}
								n_items--;
							}
							// Property written, move to next line
							file << "\n";
						} else {
							// Yes, property has properties of its own, so it's a node
							// Force a new line and write out the node's name
							file << "\n" << indent(s_indent, n_indent_count) << property.first << "\n";
							// Open braces, and update indentation
							file << indent(s_indent, n_indent_count) << "{\n";
							n_indent_count++;
							// Recursively write that node
							write(property.second, file);
							// Node written, so close braces
							file << indent(s_indent, n_indent_count) << "}\n\n";
						}
					}
					// We've finished writing out a node, regardless of state, our indentation
					// must decrease, unless we're top level
					if (n_indent_count > 0) n_indent_count--;
				}; // NOTE why the semicolon

				if (file.is_open()) {
					write(n, file); 
					return true;
				}
				return false;
			}
		private:
			// list of strings that make up a propery value
			std::vector<std::string>											m_vContent;
			// vector holds the order, pair has .first .second members
			std::vector<std::pair<std::string, DataFile>> m_vecObjects;
			// this maps the name of the child node to an index
			std::unordered_map<std::string, size_t>				m_mapObjects;
	};
} // namespace olc::utils
