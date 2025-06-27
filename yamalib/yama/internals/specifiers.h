

#pragma once


#include "../core/macros.h"

#include "env.h"


namespace yama::internal {


    // TODO: maybe employ a TAUL spec to allow for us to easily maintain a more nuanced
    //       syntax for specifiers

    class import_path;
    class unqualified_name;
    class qualified_name;
    class fullname;


    struct parsed_ip final {
        str head, relative_path;
    };
    struct parsed_uqn final {
        str name, owner_name, member_name;


        inline bool is_member() const noexcept { return !member_name.empty(); }
    };
    struct parsed_qn final {
        parsed_ip ip;
        parsed_uqn uqn;
    };
    struct parsed_fln final {
        parsed_qn qn;
    };

    std::optional<parsed_ip> parse_ip(const str& input);
    std::optional<parsed_uqn> parse_uqn(const str& input);
    std::optional<parsed_qn> parse_qn(const str& input);
    std::optional<parsed_fln> parse_fln(const str& input);


    // TODO: later, if we add heap allocs to parsing, be sure to revise below (especially
    //       valid_fln) to ensure they avoid heap usage

    bool valid_ip(const str& input);
    bool valid_uqn(const str& input);
    bool valid_qn(const str& input);
    bool valid_fln(const str& input);


    // extract_head_name extracts the head identifier portion of x

    // x may be an import path, qualified name, or fullname

    std::optional<str> extract_head_name(const str& x);


    // TODO: specifier_provider exists for us to later add in caching to reduce heap usage

    class specifier_provider final {
    public:
        specifier_provider() = default;


        std::optional<import_path> ip(const env& e, const yama::str& x, bool& head_was_bad);
        std::optional<import_path> ip(const env& e, const yama::str& x);
        
        std::optional<unqualified_name> uqn(const yama::str& x);
        
        std::optional<qualified_name> qn(const env& e, const yama::str& x, bool& head_was_bad);
        std::optional<qualified_name> qn(const env& e, const yama::str& x);

        std::optional<fullname> fln(const env& e, const yama::str& x, bool& head_was_bad);
        std::optional<fullname> fln(const env& e, const yama::str& x);
    };


    // IMPORTANT: specifiers are intended ONLY for use within compiler, to help reduce
    //            the complexity of dealing w/ names across different parcel envs
    //              * TODO: this *rule* is violated by the fact that loader uses specifiers,
    //                      so maybe better rule would be that these are only for use by
    //                      things like compiler/loader/etc. behind a common mutex

    // IMPORTANT: these 'specifiers' are intended to be *env agnostic*, meaning that they
    //            use parcel IDs rather than name identifiers to encode parcel association,
    //            w/ envs only being relevant upon parse/format

    // TODO: the below classes are all fairly 'fat', and we can likely GREATLY improve
    //       them in terms of space efficiency


    class import_path final {
    public:
        import_path() = delete;
        import_path(const import_path&) = default;
        import_path(import_path&&) noexcept = default;
        ~import_path() noexcept = default;
        import_path& operator=(const import_path&) = default;
        import_path& operator=(import_path&&) noexcept = default;


        parcel_id head() const noexcept;
        const yama::str& relative_path() const noexcept;
        bool is_root() const noexcept;

        size_t hash() const noexcept;

        yama::str str() const noexcept;
        yama::str str(const env& e) const noexcept;

        std::string fmt() const noexcept;
        std::string fmt(const env& e) const noexcept;


        bool operator==(const import_path&) const noexcept = default;


        // head_was_bad is set to true if parse fails due to invalid head, and is
        // set to false in all other cases (including success)

        static std::optional<import_path> parse(const env& e, const yama::str& x, bool& head_was_bad);
        static std::optional<import_path> parse(const env& e, const yama::str& x);


    private:
        parcel_id _head;
        yama::str _relative_path;


        import_path(parcel_id head, yama::str&& relative_path);
    };


    class unqualified_name final {
    public:
        unqualified_name(const str& name);
        unqualified_name(const str& owner_name, const str& member_name);

        unqualified_name() = delete;
        unqualified_name(const unqualified_name&) = default;
        unqualified_name(unqualified_name&&) noexcept = default;
        ~unqualified_name() noexcept = default;
        unqualified_name& operator=(const unqualified_name&) = default;
        unqualified_name& operator=(unqualified_name&&) noexcept = default;


        str owner_name() const noexcept; // equiv to uqn if non-member
        str member_name() const noexcept;
        bool is_member() const noexcept;

        size_t hash() const noexcept;
        bool operator==(const unqualified_name&) const noexcept = default;

        yama::str str() const noexcept;
        yama::str str(const env& e) const noexcept;

        std::string fmt() const noexcept;
        std::string fmt(const env& e) const noexcept;


        static std::optional<unqualified_name> parse(const yama::str& x);


    private:
        yama::str _name;
    };


    class qualified_name final {
    public:
        qualified_name(internal::import_path ip, internal::unqualified_name uqn);

        qualified_name() = delete;
        qualified_name(const qualified_name&) = default;
        qualified_name(qualified_name&&) noexcept = default;
        ~qualified_name() noexcept = default;
        qualified_name& operator=(const qualified_name&) = default;
        qualified_name& operator=(qualified_name&&) noexcept = default;


        const yama::internal::import_path& ip() const noexcept;
        const yama::internal::unqualified_name& uqn() const noexcept;

        inline operator const yama::internal::import_path&() const noexcept { return ip(); }
        inline operator const yama::internal::unqualified_name&() const noexcept { return uqn(); }

        size_t hash() const noexcept;
        bool operator==(const qualified_name&) const noexcept = default;

        yama::str str() const noexcept;
        yama::str str(const env& e) const noexcept;

        std::string fmt() const noexcept;
        std::string fmt(const env& e) const noexcept;


        // head_was_bad is set to true if parse fails due to invalid head, and is
        // set to false in all other cases (including success)

        static std::optional<qualified_name> parse(const env& e, const yama::str& x, bool& head_was_bad);
        static std::optional<qualified_name> parse(const env& e, const yama::str& x);


    private:
        yama::internal::import_path _ip;
        yama::internal::unqualified_name _uqn;
    };


    // TODO: when we add generics, be sure to look into ways we can use caching to avoid
    //       unneeded heap allocs
    //
    //       something like a 'yama::specifier_cache' which gets injected into parse, or
    //       maybe we could use thread-local caching in the impl
    //
    //       it could also help to figure out ways to reduce heap allocs down to one, by
    //       computing the blocks we'll need, and then allocating a single big block,
    //       everything we need onto this one big block

    class fullname final {
    public:
        fullname(internal::qualified_name qn);

        fullname() = delete;
        fullname(const fullname&) = default;
        fullname(fullname&&) noexcept = default;
        ~fullname() noexcept = default;
        fullname& operator=(const fullname&) = default;
        fullname& operator=(fullname&&) noexcept = default;


        const yama::internal::import_path& ip() const noexcept;
        const yama::internal::unqualified_name& uqn() const noexcept;
        const yama::internal::qualified_name& qn() const noexcept;

        inline operator const yama::internal::import_path&() const noexcept { return ip(); }
        inline operator const yama::internal::unqualified_name&() const noexcept { return uqn(); }
        inline operator const yama::internal::qualified_name&() const noexcept { return qn(); }

        size_t hash() const noexcept;
        bool operator==(const fullname&) const noexcept = default;

        yama::str str() const noexcept;
        yama::str str(const env& e) const noexcept;

        std::string fmt() const noexcept;
        std::string fmt(const env& e) const noexcept;


        // head_was_bad is set to true if parse fails due to invalid head, and is
        // set to false in all other cases (including success)

        static std::optional<fullname> parse(const env& e, const yama::str& x, bool& head_was_bad);
        static std::optional<fullname> parse(const env& e, const yama::str& x);


    private:
        yama::internal::qualified_name _qn;
    };
}


YAMA_SETUP_HASH(yama::internal::import_path, x.hash());
YAMA_SETUP_HASH(yama::internal::unqualified_name, x.hash());
YAMA_SETUP_HASH(yama::internal::qualified_name, x.hash());
YAMA_SETUP_HASH(yama::internal::fullname, x.hash());

YAMA_SETUP_FORMAT(yama::internal::import_path, x.fmt());
YAMA_SETUP_FORMAT(yama::internal::unqualified_name, x.fmt());
YAMA_SETUP_FORMAT(yama::internal::qualified_name, x.fmt());
YAMA_SETUP_FORMAT(yama::internal::fullname, x.fmt());

