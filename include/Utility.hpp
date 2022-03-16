#pragma once

#include <cstring>
#include <cctype>
#include <map>

namespace flint
{
	namespace utility
	{
		struct CaseInsensitiveComparator
		{
			bool operator() (const char* s1, const char* s2) const	{ return _stricmp(s1, s2); }
		};

		class CommandLineArguments
		{
		public:
			
			explicit CommandLineArguments(char* args)
			{
				parse(args);
			}

			const char* get(const char* key) const
			{
				auto itr = m_args.find(key);
				return (itr != m_args.cend()) ? itr->second : nullptr;
			}

			bool has(const char* key) const
			{
				return m_args.find(key) != m_args.cend();
			}

		private:

			CommandLineArguments() = delete;
			std::map<const char*, const char*, CaseInsensitiveComparator> m_args;

			void parse(char* cmd)
			{
				bool bIgnoreWS = true;
				char nToken = 0;
				auto itr = m_args.end();
				const char* buffer = "";
				while (true)
				{
					const char ch = *cmd;
					if (ch == 0 || isspace(ch))
					{
						if (bIgnoreWS)
						{
							++cmd;
							continue;
						}
						if (nToken == 0)
						{
							itr = m_args.insert(std::make_pair(buffer, cmd)).first;
							nToken = 1;
						}
						else if (nToken == 2)
						{
							itr->second = buffer;
							nToken = 0;
						}
						if (ch == 0)
							break;
						*cmd = 0;
						buffer = cmd;
						bIgnoreWS = true;
					}
					else if (ch == '=')
					{
						if (nToken == 2)
						{
							*cmd = 0;
							itr->second = buffer;
							break;
						}
						else if (nToken == 0)
						{
							if (*buffer == 0)
								break;
							itr = m_args.insert(std::make_pair(buffer, cmd)).first;
							*cmd = 0;
							buffer = cmd;
						}
						nToken = 2;
						bIgnoreWS = true;
					}
					else if (ch == '"')
					{
						*cmd = 0;
						if (nToken == 2)
						{
							if (*buffer == 0)
							{
								buffer = ++cmd;
								while (*cmd != 0 && *cmd != '"')
									++cmd;
								if (*cmd == 0)
									break;
								*cmd = 0;
								itr->second = buffer;
								++cmd;
								if (*cmd == 0 || *cmd != ' ')
									break;
								*cmd = 0;
								buffer = cmd;
								nToken = 0;
								bIgnoreWS = true;
							}
							else
								break;
						}
						else
							break;
					}
					else if (*buffer == 0)
					{
						bIgnoreWS = false;
						if (nToken == 1)
							nToken = 0;
						buffer = cmd;
					}
					++cmd;
				}
			}

		};
	}
}