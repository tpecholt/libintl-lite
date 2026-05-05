#include "libintl.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>
#include <algorithm>
#include <limits.h>

namespace
{

// Plural form Evaluator

struct PluralFormEvaluator
{
	void Init(const std::string& plural)
	{
		Tokenize(plural);
		if (tokens.size() && tokens[0] == LPAR && tokens.back() == RPAR) {
			tokens.erase(tokens.begin());
			tokens.pop_back();
		}
	}
	int Eval(size_t n)
	{
		pos = 0;
		nvalue = n > INT_MAX ? INT_MAX : (int)n;
		try {
			return E();
		}
		catch (const std::exception&) {
			return -1;
		}
	}

private:
	void Tokenize(const std::string& str)
	{
		tokens.clear();
		int state = 0;
		for (size_t i = 0; i < str.size(); ++i) {
			char c = str[i];
			char cn = i + 1 < str.size() ? str[i + 1] : 0;
			if (!state) {
				if (c == 'n')
					tokens.push_back(VAR);
				else if (c == '(')
					tokens.push_back(LPAR);
				else if (c == ')')
					tokens.push_back(RPAR);
				else if (c == '?')
					tokens.push_back(QMARK);
				else if (c == ':')
					tokens.push_back(COLON);
				else if (c == '%')
					tokens.push_back(MODULO);
				else if (c == '=' && cn == '=') {
					tokens.push_back(EQ);
					++i;
				}
				else if (c == '!' && cn == '=') {
					tokens.push_back(NEQ);
					++i;
				}
				else if (c == '<') {
					if (cn == '=') {
						tokens.push_back(LE);
						++i;
					}
					else
						tokens.push_back(LT);
				}
				else if (c == '>') {
					if (cn == '=') {
						tokens.push_back(GE);
						++i;
					}
					else
						tokens.push_back(GE);
				}
				else if (c == '&' && cn == '&') {
					tokens.push_back(AND);
					++i;
				}
				else if (c == '|' && cn == '|') {
					tokens.push_back(OR);
					++i;
				}
				else if (c >= '0' && c <= '9') {
					state = 1;
					tokens.push_back(c - '0');
				}
				else if (c == ' ') {
				}
				else {
					//error
					tokens.clear();
					return;
				}
			}
			else
			{
				if (c >= '0' && c <= '9') {
					tokens.back() = tokens.back() * 10 + c - '0';
				}
				else {
					state = 0;
					--i;
				}
			}
		}
	}
	int Next()
	{
		return pos < tokens.size() ? tokens[pos] : END;
	}
	void Match(int token)
	{
		if (Next() != token)
			throw std::invalid_argument("");
		if (pos < tokens.size())
			++pos;
	}
	int F()
	{
		int value, next;
		switch(next = Next()) {
		case LPAR:
			Match(LPAR);
			value = E();
			Match(RPAR);
			return value;
		case VAR:
			Match(VAR);
			return nvalue;
		default:
			if (next >= 0) {
				Match(next);
				return next;
			}
			throw std::invalid_argument("");
		}
	}
	int R1(int leftvalue)
	{
		int next = Next();
		int value;
		switch (next) {
		case EQ:
		case NEQ:
		case GT:
		case GE:
		case LT:
		case LE:
			Match(next);
			value = F();
			if (next == EQ)
				return leftvalue == value;
			else if (next == NEQ)
				return leftvalue != value;
			else if (next == GT)
				return leftvalue > value;
			else if (next == GE)
				return leftvalue >= value;
			else if (next == LT)
				return leftvalue < value;
			else //if (next == LE)
				return leftvalue <= value;
		case MODULO:
			Match(next);
			value = F();
			return R1(leftvalue % value);
		default:
			return leftvalue;
		}
	}
	int R()
	{
		int value = F();
		return R1(value);
	}
	int C()
	{
		int value = R();
		return C1(value);
	}
	int C1(int leftvalue)
	{
		int value;
		switch (Next()) {
		case AND:
			Match(AND);
			value = R();
			return leftvalue && value;
		default:
			return leftvalue;
		}
	}
	int D()
	{
		int value = C();
		return D1(value);
	}
	int D1(int leftvalue)
	{
		int value;
		switch (Next()) {
		case OR:
			Match(OR);
			value = C();
			return leftvalue || value;
		default:
			return leftvalue;
		}
	}
	int E()
	{
		int value = D();
		return E1(value);
	}
	int E1(int leftvalue)
	{
		int value1, value2;
		switch (Next()) {
		case QMARK:
			Match(QMARK);
			value1 = E();
			Match(COLON);
			value2 = E();
			return leftvalue ? value1 : value2;
		default:
			return leftvalue;
		}
	}

	enum Token { END = -100, EQ, NEQ, LT, GT, LE, GE, AND, OR, MODULO, VAR, LPAR, RPAR, QMARK, COLON };
	std::vector<int> tokens;
	size_t pos;
	int nvalue;
};

// Message Catalog

struct MessageCatalog
{
	struct TranslatedString
	{
		std::string_view str;
		std::vector<std::string_view> pluralForms;
	};

	//-------------------------------------------

	const std::string& GetDomain() const
	{
		return domain;
	}

	const std::string& GetLanguage() const
	{
		return language;
	}

	int EvalPluralForm(size_t n)
	{
		return pfe.Eval(n);
	}

	const TranslatedString* Translate(std::string_view context, std::string_view str)
	{
		OrigString orig{ context, str };
		auto it = std::lower_bound(origStrings.begin(), origStrings.end(), orig);
		if (it == origStrings.end() || *it != orig)
			return nullptr;
		size_t i = it - origStrings.begin();
		return &transStrings[i];
	}

	int Load(std::string_view domain, std::string_view moFilePath)
	{
		std::ifstream fin(u8path(moFilePath), std::ios::binary);
		if (!fin)
			return 1;

		int magicNumber;
		if (!fin.read((char*)&magicNumber, 4))
			return 1;
		if (magicNumber != 0x950412de && magicNumber != 0xde120495)
			return 2;

		int formatVersion;
		if (!fin.read((char*)&formatVersion, 4) || formatVersion != 0)
			return 2;

		int num = 0;
		if (!fin.read((char*)&num, 4) || !num)
			return 2;

		uint32_t offsetOrigTable = 0;
		if (!fin.read((char*)&offsetOrigTable, 4))
			return 2;
		uint32_t offsetTransTable = 0;
		if (!fin.read((char*)&offsetTransTable, 4))
			return 2;

		this->domain = domain;

		//read orig strings
		uint32_t firstStringOffset;
		std::vector<uint32_t> stringLens;
		fin.seekg(offsetOrigTable);
		uint32_t nbytes = ReadStringTable(fin, num, firstStringOffset, stringLens);
		fin.seekg(firstStringOffset);
		origBlock.resize(nbytes);
		fin.read(origBlock.data(), nbytes);
		origStrings.resize(num);
		const char* ptr = origBlock.data();
		for (int i = 0; i < num; ++i)
		{
			OrigString& g = origStrings[i];
			g.str = { ptr, stringLens[i] };
			size_t j = g.str.find('\x4');
			if (j != std::string_view::npos) {
				g.context = g.str.substr(0, j);
				g.str.remove_prefix(j + 1);
				//ptr[j] = 0;
			}
			j = g.str.find('\0');
			if (j != std::string_view::npos)
				g.str.remove_suffix(g.str.size() - j);
			ptr += stringLens[i] + 1;
		}

		//read translated strings
		std::vector<uint32_t> transStringLens;
		fin.seekg(offsetTransTable);
		nbytes = ReadStringTable(fin, num, firstStringOffset, stringLens);
		fin.seekg(firstStringOffset);
		transBlock.resize(nbytes);
		fin.read(transBlock.data(), nbytes);
		transStrings.resize(num);
		ptr = transBlock.data();
		if (origStrings[0].str.empty())
		{
			//parse header stored in the first translation record
			std::string line;
			std::istringstream is(std::string(ptr, stringLens[0]));
			while (std::getline(is, line)) {
				if (!line.compare(0, 9, "Language:")) {
					size_t i = 9;
					while (i < line.size() && std::isspace(line[i]))
						++i;
					language = line.substr(i);
				}
				else if (!line.compare(0, 13, "Plural-Forms:")) {
					size_t i = line.find("nplurals=");
					if (i != std::string::npos && line.size() > i + 9)
						nplurals = line[i + 9] - '0';
					i = line.find("plural=");
					if (i != std::string::npos && line.size() > i + 7) {
						size_t j = line.find(';', i);
						if (j == std::string::npos)
							j = line.size();
						pluralRule = line.substr(i + 7, j - i - 7);
						pfe.Init(pluralRule);
					}
				}
			}
		}
		for (int i = 0; i < num; ++i)
		{
			TranslatedString& t = transStrings[i];
			t.str = { ptr, stringLens[i] };
			size_t j = t.str.find('\0');
			if (j != std::string_view::npos)
			{
				t.pluralForms.reserve(5);
				t.pluralForms.push_back(t.str.substr(j + 1));
				t.str.remove_suffix(t.str.size() - j);
				while ((j = t.pluralForms.back().find('\0')) != std::string_view::npos)
				{
					std::string_view& plural = t.pluralForms.back();
					t.pluralForms.push_back(plural.substr(j + 1));
					plural.remove_suffix(plural.size() - j);
				}
			}
			ptr += stringLens[i] + 1;
		}

		return 0;
	}

private:
	struct OrigString
	{
		std::string_view context;
		std::string_view str;

		bool operator== (const OrigString& other) const
		{
			return context == other.context && str == other.str;
		}
		bool operator!= (const OrigString& other) const
		{
			return !(*this == other);
		}
		bool operator< (const OrigString& other) const
		{
			int c = context.compare(other.context);
			if (c < 0)
				return true;
			else if (c > 0)
				return false;

			return str < other.str;
		}
	};

	std::filesystem::path u8path(std::string_view s)
	{
#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || __cplusplus >= 202002L
		return std::filesystem::path((const char8_t*)s.data(), (const char8_t*)s.data() + s.size());
#else
		return std::filesystem::u8path(s);
#endif
	}

	uint32_t ReadStringTable(std::ifstream& fin, int num, uint32_t& firstOffset, std::vector<uint32_t>& stringLens)
	{
		uint32_t bytes = 0;
		stringLens.resize(num);

		if (!fin.read((char*)&stringLens[0], 4))
			return 0;
		bytes += stringLens[0] + 1;
		if (!fin.read((char*)&firstOffset, 4))
			return 0;

		for (int i = 1; i < num; ++i) {
			if (!fin.read((char*)&stringLens[i], 4))
				return 2;
			fin.ignore(4);
			bytes += stringLens[i] + 1;
		}
		return bytes;
	}

	std::vector<OrigString> origStrings;
	std::vector<TranslatedString> transStrings;
	std::vector<char> origBlock;
	std::vector<char> transBlock;

	std::string domain;
	std::string language;
	int nplurals = 0;
	std::string pluralRule;
	PluralFormEvaluator pfe;
};

// Global Data

std::string curDomain;
std::vector<std::pair<std::string, std::string>> domainRoots;
std::vector<MessageCatalog> mcs;
MessageCatalog* curMC = nullptr;

// Utilities

std::vector<std::string> GetPrefferedLanguages()
{
	std::vector<std::string> out;
	const char* plang = getenv("LANGUAGE");
	if (plang) {
		std::string lang = plang;
		std::replace(lang.begin(), lang.end(), ':', ' ');
		std::istringstream is(lang);
		while (is >> lang)
			out.push_back(lang);
		return out;
	}
	std::string lang = std::locale("").name();
	return { lang };
}

}

// Gettext API

bool LoadMessageCatalog(std::string_view domain, std::string_view moPath)
{
	curDomain = domain;
	MessageCatalog mc;
	if (mc.Load(domain, moPath))
		return false;
	auto it = std::find_if(mcs.begin(), mcs.end(), [&mc](const auto& other) {
		return other.GetDomain() == mc.GetDomain() && other.GetLanguage() == mc.GetLanguage();
		});
	if (it != mcs.end())
		*it = std::move(mc);
	else {
		mcs.push_back(std::move(mc));
		it = --mcs.end();
	}
	curMC = &*it;
	return true;
}

char* bindtextdomain(const char* domain, const char* r)
{
	std::string root = r;
	if (!root.empty() && (root.back() == '/' || root.back() == '\\'))
		root.pop_back();
	auto it = std::find_if(domainRoots.begin(), domainRoots.end(), [domain](const auto& dr) {
		return dr.first == domain;
		});
	if (it != domainRoots.end()) {
		it->second = root;
		return (char*)it->second.c_str();
	}
	else {
		domainRoots.push_back({ domain, root });
		return (char*)domainRoots.back().second.c_str();
	}
}

char* textdomain(const char* domain)
{
	if (domain)
	{
		curDomain = domain;
		curMC = nullptr;
		auto langs = GetPrefferedLanguages();
		auto dr = std::find_if(domainRoots.begin(), domainRoots.end(), [domain](const auto& dr) {
			return dr.first == domain;
			});
		std::string root = dr == domainRoots.end() ? "." : dr->second;
		for (const std::string& lang : langs)
		{
			auto it = std::find_if(mcs.begin(), mcs.end(), [&](const auto& mc) {
				return mc.GetDomain() == domain && mc.GetLanguage() == lang;
				});
			if (it != mcs.end()) {
				curMC = &*it;
				break;
			}
			else
			{
				std::string path = root + "/" + lang + "/LC_MESSAGE/" + curDomain + ".mo";
				if (LoadMessageCatalog(domain, path))
					break;
			}
		}
	}
	return (char*)curDomain.c_str();
}

char* gettext(const char* str)
{
	return (char*)pgettext("", str);
}

const char* pgettext(const char* context, const char* str)
{
	if (!curMC)
		return str;
	const auto* t = curMC->Translate(context, str);
	return t ? t->str.data() : str;
}

char* ngettext(const char* str, const char* plural, unsigned long n)
{
	return (char*)npgettext("", str, plural, n);
}

const char* npgettext(const char* context, const char* str, const char* plural, unsigned long n)
{
	if (!curMC)
		return n ? plural : str;

	const auto* t = curMC->Translate(context, str);
	if (!t)
		return n ? plural : str;

	int pn = curMC->EvalPluralForm(n);
	if (pn < 0 || pn >= 1 + t->pluralForms.size())
		return n ? plural : str;

	if (!pn)
		return t->str.data();
	else
		return t->pluralForms[pn - 1].data();
}
