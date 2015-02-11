class EncodingConvertor  {
public:
	EncodingConvertor(void);
public:
	~EncodingConvertor(void);
public:
	static bool Unicode2UTF8(const wchar_t *input_unicode,
		char ** p_output_utf8,
		unsigned long *length) {
			if (input_unicode == NULL) {
				return true;
			}
			int size_d = 8;
			int buffer_size = 0;

			const wchar_t* p_unicode = input_unicode;
			// count for the space need to allocate
			wchar_t w_char;
			do {
				w_char = *p_unicode;
				if (w_char < 0x80) {
					// utf char size is 1
					buffer_size += 1;
				} else if (w_char < 0x800) {
					// utf char size is 2
					buffer_size += 2;
				} else if (w_char < 0x10000) {
					// utf char size is 3
					buffer_size += 3;
				} else if (w_char < 0x200000) {
					// utf char size is 4
					buffer_size += 4;
				} else if (w_char < 0x4000000) {
					// utf char size is 5
					buffer_size += 5;
				} else {
					// utf char size is 6
					buffer_size += 6;
				}
				p_unicode++;
			}
			while (w_char != static_cast<char>(0));
			// allocate the memory
			char* utf8 = new char[buffer_size];

			p_unicode = input_unicode;
			int index_buffer = 0;
			// do the conversion
			do {
				w_char = *input_unicode;  // the unicode char current being converted
				input_unicode++;

				if (w_char < 0x80) {
					// length = 1;
					utf8[index_buffer++] = static_cast<char>(w_char);
				} else if (w_char < 0x800) {
					// length = 2;
					utf8[index_buffer++] = 0xc0 | (w_char >> 6);
					utf8[index_buffer++] = 0x80 | (w_char & 0x3f);
				} else if (w_char < 0x10000) {
					// length = 3;
					utf8[index_buffer++] = 0xe0 | (w_char >> 12);
					utf8[index_buffer++] = 0x80 | ((w_char >> 6) & 0x3f);
					utf8[index_buffer++] = 0x80 | (w_char & 0x3f);
				} else if (w_char < 0x200000) {
					// length = 4;
					utf8[index_buffer++] = 0xf0 | (static_cast<int>(w_char) >> 18);
					utf8[index_buffer++] = 0x80 | ((w_char >> 12) & 0x3f);
					utf8[index_buffer++] = 0x80 | ((w_char >> 6) & 0x3f);
					utf8[index_buffer++] = 0x80 | (w_char & 0x3f);
				} else if (w_char < 0x4000000) {
					// length = 5
					utf8[index_buffer++] = 0xf8| (static_cast<int>(w_char) >> 24);
					utf8[index_buffer++] = 0x80 | ((static_cast<int>(w_char) >> 18) & 0x3f);
					utf8[index_buffer++] = 0x80 | ((w_char >> 12) & 0x3f);
					utf8[index_buffer++] = 0x80 | ((w_char >> 6) & 0x3f);
					utf8[index_buffer++] = 0x80 | (w_char & 0x3f);
				} else {  // if(wchar >= 0x4000000)
					// all other cases length = 6
					utf8[index_buffer++] = 0xfc | (static_cast<int>(w_char) >> 30);
					utf8[index_buffer++] = 0x80 | ((static_cast<int>(w_char) >> 24) & 0x3f);
					utf8[index_buffer++] = 0x80 | ((static_cast<int>(w_char) >> 18) & 0x3f);
					utf8[index_buffer++] = 0x80 | ((w_char >> 12) & 0x3f);
					utf8[index_buffer++] = 0x80 | ((w_char >> 6) & 0x3f);
					utf8[index_buffer++] = 0x80 | (w_char & 0x3f);
				}
			}
			while (w_char !=  static_cast<char>(0));

			// set the output length
			*length = buffer_size - 1;  // ignore last 

			// set the output charset
			*p_output_utf8 = utf8;
			return false;
	}

	static bool UTF82Unicode(const char *input_utf8,
		wchar_t ** p_output_unicode,
		unsigned long *length) {
			if (input_utf8 == NULL) {  // input wrong.
				return false;
			}

			const char* p_current_char = input_utf8;
			unsigned long unicode_length = 0;
			char current_char;
			// calculate the size to locate
			do {
				// get the begining char
				current_char = *p_current_char;

				if ((current_char  & 0x80) == 0) {
					// 0xxxxxxx
					p_current_char++;
				} else if ((current_char  & 0xe0) == 0xc0) {
					// < 110x-xxxx 10xx-xxxx
					p_current_char += 2;
				} else if ((current_char  & 0xf0) == 0xe0) {
					// < 1110-xxxx 10xx-xxxx 10xx-xxxx
					p_current_char += 3;
				} else if ((current_char  & 0xf8) == 0xf0) {
					// < 1111-0xxx 10xx-xxxx 10xx-xxxx 10xx-xxxx
					p_current_char += 4;

				} else if ((current_char & 0xfc) == 0xf8) {
					// 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
					p_current_char += 5;
				} else {
					// if((current_char & 0xfe) == 0xfc)
					// 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
					p_current_char += 6;
				}
				unicode_length++;
			}
			while (current_char != 0);

			wchar_t* des = new wchar_t[unicode_length];
			unsigned long unicode_index = 0;
			p_current_char = input_utf8;

			do {
				current_char = *p_current_char;

				if ((current_char & 0x80) == 0) {
					des[unicode_index] = p_current_char[0];

					p_current_char++;
				} else if ((current_char & 0xE0) == 0xC0) {
					// < 110x-xxxx 10xx-xxxx
					wchar_t &wide_char = des[unicode_index];
					wide_char  = (p_current_char[0] & 0x3F) << 6;
					wide_char |= (p_current_char[1] & 0x3F);

					p_current_char += 2;
				} else if ((current_char & 0xF0) == 0xE0) {
					// < 1110-xxxx 10xx-xxxx 10xx-xxxx
					wchar_t &wide_char = des[unicode_index];

					wide_char  = (p_current_char[0] & 0x1F) << 12;
					wide_char |= (p_current_char[1] & 0x3F) << 6;
					wide_char |= (p_current_char[2] & 0x3F);

					p_current_char += 3;
				} else if ((current_char & 0xF8) == 0xF0) {
					// < 1111-0xxx 10xx-xxxx 10xx-xxxx 10xx-xxxx
					wchar_t &wide_char = des[unicode_index];

					wide_char  = (p_current_char[0] & 0x0F) << 18;
					wide_char |= (p_current_char[1] & 0x3F) << 12;
					wide_char |= (p_current_char[2] & 0x3F) << 6;
					wide_char |= (p_current_char[3] & 0x3F);

					p_current_char += 4;
				} else if ((current_char & 0xfc) == 0xf8) {
					// 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
					wchar_t &wide_char = des[unicode_index];

					wide_char = (p_current_char[0] & 0x07) << 24;
					wide_char |= (p_current_char[1] & 0x3F) << 18;
					wide_char |= (p_current_char[2] & 0x3F) << 12;
					wide_char |= (p_current_char[3] & 0x3F) << 6;
					wide_char |= (p_current_char[4] & 0x3F);

					p_current_char += 5;
				} else {
					// if((*current_char & 0xfe) == 0xfc)
					// 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

					wchar_t &wide_char = des[unicode_index];

					wide_char = (p_current_char[0] & 0x03) << 30;
					wide_char |= (p_current_char[1] & 0x3F) << 24;
					wide_char |= (p_current_char[2] & 0x3F) << 18;
					wide_char |= (p_current_char[3] & 0x3F) << 12;
					wide_char |= (p_current_char[4] & 0x3F) << 6;
					wide_char |= (p_current_char[5] & 0x3F);
					p_current_char += 6;
				}
				unicode_index++;
			} while (current_char != 0);

			*p_output_unicode =  des;
			*length = unicode_length - 1;  // ignore the last 

			return true;
	}

	static bool  UTF82UrlEncode(const char* input_utf8,
		char ** p_url_encode,
		unsigned long *length) {
			unsigned long total_length = 0;
			// calculate output size
			const char* current_pos = input_utf8;
			do {
				// the char needs to be encode
				if (!((*current_pos) >= 'a' && (*current_pos) <= 'z') &&
					!((*current_pos) >= 'A' && (*current_pos) <= 'Z') &&
					!((*current_pos) >= '0' && (*current_pos) <= '9') &&
					!((*current_pos) == ' ') &&
					!((*current_pos) == '.') &&
					!((*current_pos) == '-') &&
					!((*current_pos) == '_') &&
					!((*current_pos) == '~') &&
					!((*current_pos) == '\'') &&
					!((*current_pos) == '(') &&
					!((*current_pos) == ')') &&
					!((*current_pos) == '*') &&
					!((*current_pos) == 0)
					) {
						total_length += 3;
				} else {
					total_length++;
				}
				if ((*current_pos) == 0) {  // the end of str
					break;
				}
				current_pos++;

			} while (true);
			char* output_buffer = new char[total_length];
			current_pos = input_utf8;
			char* target_current_pos = output_buffer;
			unsigned long left_size = total_length;
			// do conversion
			do {
				// the char needs to be encode
				if (!((*current_pos) >= 'a' && (*current_pos) <= 'z') &&
					!((*current_pos) >= 'A' && (*current_pos) <= 'Z') &&
					!((*current_pos) >= '0' && (*current_pos) <= '9') &&
					!((*current_pos) == ' ') &&
					!((*current_pos) == '.') &&
					!((*current_pos) == '-') &&
					!((*current_pos) == '_') &&
					!((*current_pos) == '~') &&
					!((*current_pos) == '\'') &&
					!((*current_pos) == '(') &&
					!((*current_pos) == ')') &&
					!((*current_pos) == '*') &&
					!((*current_pos) == 0)
					) {
						unsigned char temp_unsigned_char = *current_pos;
						// convert to unsigned to skip the sign bit

						::_snprintf_s(target_current_pos,
							left_size,
							_TRUNCATE ,
							"%%%02X",
							temp_unsigned_char);
						target_current_pos += 3;
						left_size -= 3;
				} else if ((*current_pos) ==  ' ') {
					// the char blank
					*target_current_pos = '+';
					target_current_pos++;
					left_size--;
				} else {
					// char that need not change
					*target_current_pos = *current_pos;
					target_current_pos++;
					left_size--;
				}
				if ((*current_pos) == 0) {
					break;
				}
				current_pos++;
			} while (true);

			*p_url_encode = output_buffer;
			*length = total_length - 1;  // ignore last 
			return true;
	}
};

EncodingConvertor::EncodingConvertor(void) {
}

EncodingConvertor::~EncodingConvertor(void) {
}

