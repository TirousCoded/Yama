

#pragma once


#include "macros.h"
#include "env.h"


namespace yama {


    // TODO: these classes have not been unit tested

    // TODO: when we do unit test + revise these, be sure to look into better formalizing
    //       their parsing/formatting semantics
    //
    //       as part of this, maybe employ a TAUL spec to allow for us to easily maintain
    //       a more nuanced syntax for specifiers

    class import_path;
    class qualified_name;
    class fullname;


    struct parsed_import_path final {
        str head, relative_path;
    };
    struct parsed_qualified_name final {
        parsed_import_path import_path;
        str unqualified_name;
    };
    struct parsed_fullname final {
        parsed_qualified_name qualified_name;
    };

    std::optional<parsed_import_path> parse_import_path(const str& input);
    std::optional<parsed_qualified_name> parse_qualified_name(const str& input);
    std::optional<parsed_fullname> parse_fullname(const str& input);


    // IMPORTANT: these 'specifiers' are intended to be *env agnostic*, meaning that they
    //            use parcel IDs rather than name identifiers to encode parcel association,
    //            w/ envs only being relevant upon parse/format


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


    class qualified_name final {
    public:
        qualified_name() = delete;
        qualified_name(const qualified_name&) = default;
        qualified_name(qualified_name&&) noexcept = default;

        ~qualified_name() noexcept = default;

        qualified_name& operator=(const qualified_name&) = default;
        qualified_name& operator=(qualified_name&&) noexcept = default;


        const yama::import_path& import_path() const noexcept;
        inline operator const yama::import_path&() const noexcept { return import_path(); }

        const str& unqualified_name() const noexcept;

        parcel_id head() const noexcept;
        const yama::str& relative_path() const noexcept;
        bool is_root() const noexcept;

        size_t hash() const noexcept;

        yama::str str() const noexcept;
        yama::str str(const env& e) const noexcept;

        std::string fmt() const noexcept;
        std::string fmt(const env& e) const noexcept;


        bool operator==(const qualified_name&) const noexcept = default;


        // head_was_bad is set to true if parse fails due to invalid head, and is
        // set to false in all other cases (including success)

        static std::optional<qualified_name> parse(const env& e, const yama::str& x, bool& head_was_bad);
        static std::optional<qualified_name> parse(const env& e, const yama::str& x);


    private:
        yama::import_path _import_path;
        yama::str _unqualified_name;


        qualified_name(yama::import_path&& import_path, yama::str&& unqualified_name);
    };


    // TODO: when we add generics, be sure to look into ways we can use caching to avoid
    //       unneeded heap allocs
    //
    //       something like a 'yama::specifier_cache' which gets injected into parse, or
    //       maybe we could use thread-local caching in the impl

    class fullname final {
    public:
        fullname() = delete;
        fullname(const fullname&) = default;
        fullname(fullname&&) noexcept = default;

        ~fullname() noexcept = default;

        fullname& operator=(const fullname&) = default;
        fullname& operator=(fullname&&) noexcept = default;


        const yama::qualified_name& qualified_name() const noexcept;
        inline operator const yama::qualified_name&() const noexcept { return qualified_name(); }

        const yama::import_path& import_path() const noexcept;
        inline operator const yama::import_path&() const noexcept { return import_path(); }

        const str& unqualified_name() const noexcept;

        parcel_id head() const noexcept;
        const yama::str& relative_path() const noexcept;
        bool is_root() const noexcept;

        size_t hash() const noexcept;

        yama::str str() const noexcept;
        yama::str str(const env& e) const noexcept;

        std::string fmt() const noexcept;
        std::string fmt(const env& e) const noexcept;


        bool operator==(const fullname&) const noexcept = default;


        // head_was_bad is set to true if parse fails due to invalid head, and is
        // set to false in all other cases (including success)

        static std::optional<fullname> parse(const env& e, const yama::str& x, bool& head_was_bad);
        static std::optional<fullname> parse(const env& e, const yama::str& x);


    private:
        yama::qualified_name _qualified_name;


        fullname(yama::qualified_name&& qualified_name);
    };
}


YAMA_SETUP_HASH(yama::import_path, x.hash());
YAMA_SETUP_HASH(yama::qualified_name, x.hash());
YAMA_SETUP_HASH(yama::fullname, x.hash());

YAMA_SETUP_FORMAT(yama::import_path, x.fmt());
YAMA_SETUP_FORMAT(yama::qualified_name, x.fmt());
YAMA_SETUP_FORMAT(yama::fullname, x.fmt());

