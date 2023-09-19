#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <stack>
#include <vector>
#include <unordered_map>

inline bool is_digit(char src) {
	return (src >= '0' && src <= '9') ? true : false;
}

inline bool m_is_digit(const char *src) {
	return ((*src >= '0' && *src <= '9') || (*(src+1) >= '0' && *(src+1) <= '9')) ? true : false;
}


namespace JSON {
	typedef enum {
		STRING,
		NUMBER,
		OPERATOR,
		BRACKET
	}lexical_token;
	
	typedef struct {
		lexical_token type;
		std::string value;
	}token;
	
	union JSON_VAL;

	typedef union JSON_VAL{
		std::string str;
		long u64;
		int u32;
		char u8;
		//std::pair<JSON_VAL, JSON_VAL> obj;
		JSON_VAL() : str() { }
		~JSON_VAL() {} // OOP shit like this makes me want to live in a cabin in the woods and become amish
	}JSON_VAL;

	typedef struct JSON_OBJ{
		JSON_VAL val;
		JSON_OBJ* next;

		JSON_OBJ(): val(), next(nullptr) {}
		~JSON_OBJ() {
			JSON_OBJ* tmp;
			while (next->next != nullptr) {
				tmp = next;
				next = next->next;
				delete tmp;
			}

			delete next;
		}
	}JSON_OBJ;

	class JSON_Codec {
	private:
		std::unordered_map<char, char> order;
	public:
		JSON_Codec() {
			order[']'] = '[';
			order['}'] = '{';
		}
		
		std::vector<token> tokenize(std::string input) {
			std::vector<token> tokens;
			for (char &c: input) {
				std::cout << (int)c << std::endl;
			}

			return tokens;
		}

		JSON_OBJ *parse(std::string input) {
			std::vector<token> tokens = tokenize()

			return nullptr;	
			/*
			std::stack<char> brackets;
			JSON_OBJ *buffer = new JSON_OBJ();
			JSON_OBJ *front = buffer;
	
			for (int i = 0; i < input.size(); i++) {
				switch (input[i]) {
					case ' ':
						continue;

					case '"':
						while (input[++i] != '"') {
							buffer->val.str.push_back(input[i]);
						}
						break;

					case ',':
						buffer->next = new JSON_OBJ();
						buffer = buffer->next;
						break;

					case '[':
						brackets.push('[');
						break;

					case ']':
						std::cout << "check" << std::endl;
						if (brackets.top() != '[')
							goto error;
						else
							brackets.pop();
						break;
					
					case '{':
						brackets.push('{');
						break;

					case '}':
						std::cout << "check" << std::endl;
						if (brackets.top() != '{')
							goto error;
						else
							brackets.pop();
						break;

					case ':':
						break; 

					default: //current value is either boolean or integer
						if (m_is_digit(input.c_str())) { // is integer 
							long number = 0;
							for (int j = i; j < input.size(); j++) {
								;;
							}
						}

						break;
				}
			}
			if (brackets.size() != 0)
				goto error;

			return front;
			error:
				std::cout << "ERROR PARSING JSON FILE\n";
				// parsing error
				return front;
		}

		std::string form(JSON_OBJ* src) {
		*/

		}

		void display(JSON_OBJ* head) {
			JSON_OBJ* next = head;
			while (next != nullptr) {
				std::cout << next->val.str << std::endl;
				next = next->next;
			}
		}
	};
};
