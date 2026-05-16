#include <cstring>
#include <memory>
#include <regex>
#include <string>

#include "regex.h"

namespace {

struct RegexImpl {
    explicit RegexImpl(std::regex compiled)
        : compiled(std::move(compiled))
    {
    }

    std::regex compiled;
};

static const char *
regex_error_message(int errcode)
{
    switch (errcode) {
    case REG_NOERROR:
        return "success";
    case REG_NOMATCH:
        return "no match";
    case REG_BADPAT:
        return "invalid regular expression";
    case REG_ESPACE:
        return "memory exhausted";
    default:
        return "unknown regex error";
    }
}

static std::regex_constants::syntax_option_type
regex_syntax_options(int cflags)
{
    auto options = std::regex_constants::extended;
    if ((cflags & REG_ICASE) != 0)
        options |= std::regex_constants::icase;
    return options;
}

static std::regex_constants::match_flag_type
regex_match_flags(int eflags)
{
    auto flags = std::regex_constants::match_default;
    if ((eflags & REG_NOTBOL) != 0)
        flags |= std::regex_constants::match_not_bol;
    if ((eflags & REG_NOTEOL) != 0)
        flags |= std::regex_constants::match_not_eol;
    return flags;
}

} // namespace

extern "C" int
regcomp(regex_t *preg, const char *pattern, int cflags)
{
    try {
        auto impl = std::make_unique<RegexImpl>(
            std::regex(pattern, regex_syntax_options(cflags)));
        preg->re_nsub = impl->compiled.mark_count();
        preg->re_cflags = cflags;
        preg->re_impl = impl.release();
        return REG_NOERROR;
    } catch (const std::bad_alloc &) {
        return REG_ESPACE;
    } catch (const std::regex_error &) {
        return REG_BADPAT;
    }
}

extern "C" int
regexec(const regex_t *preg, const char *string, size_t nmatch,
    regmatch_t pmatch[], int eflags)
{
    const RegexImpl *impl;
    const char *begin;
    const char *end;
    std::cmatch match;
    bool ok;

    impl = static_cast<const RegexImpl *>(preg->re_impl);
    if (impl == nullptr)
        return REG_BADPAT;

    begin = string;
    if ((eflags & REG_STARTEND) != 0 && pmatch != nullptr) {
        begin = string + pmatch[0].rm_so;
        end = string + pmatch[0].rm_eo;
    } else {
        end = string + std::strlen(string);
    }

    try {
        ok = std::regex_search(begin, end, match, impl->compiled,
            regex_match_flags(eflags));
    } catch (const std::regex_error &) {
        return REG_BADPAT;
    }

    if (!ok)
        return REG_NOMATCH;

    if (pmatch != nullptr && (preg->re_cflags & REG_NOSUB) == 0) {
        size_t i;
        regoff_t base;

        base = static_cast<regoff_t>(begin - string);
        for (i = 0; i < nmatch; ++i) {
            if (i < match.size() && match[i].matched) {
                pmatch[i].rm_so = base + static_cast<regoff_t>(match.position(i));
                pmatch[i].rm_eo = pmatch[i].rm_so +
                    static_cast<regoff_t>(match.length(i));
            } else {
                pmatch[i].rm_so = -1;
                pmatch[i].rm_eo = -1;
            }
        }
    }

    return REG_NOERROR;
}

extern "C" size_t
regerror(int errcode, const regex_t *, char *errbuf, size_t errbuf_size)
{
    const char *msg;
    size_t needed;

    msg = regex_error_message(errcode);
    needed = std::strlen(msg) + 1;
    if (errbuf != nullptr && errbuf_size != 0) {
        size_t copylen;

        copylen = needed;
        if (copylen > errbuf_size)
            copylen = errbuf_size;
        std::memcpy(errbuf, msg, copylen - 1);
        errbuf[copylen - 1] = '\0';
    }
    return needed;
}

extern "C" void
regfree(regex_t *preg)
{
    delete static_cast<RegexImpl *>(preg->re_impl);
    preg->re_impl = nullptr;
    preg->re_nsub = 0;
    preg->re_cflags = 0;
}
