

#pragma once


#include "../core/general.h"
#include "../core/res.h"
#include "../core/parcel.h"


namespace yama::internal {


    // TODO: env[_instance] has NOT been unit tested yet

    class env;
    class env_instance;


    // env is a handle to an environment which maps name identifiers to parcel IDs

    class env final {
    public:
        env();
        env(const env&) = default;
        env(env&&) noexcept = default;

        ~env() noexcept = default;

        env& operator=(const env&) = default;
        env& operator=(env&&) noexcept = default;


        size_t count() const noexcept;
        bool exists(const str& name) const noexcept;
        bool exists(parcel_id id) const noexcept;

        std::optional<parcel_id> id(const str& name) const noexcept;
        std::optional<str> name(parcel_id id) const noexcept;

        bool operator==(const env&) const noexcept = default; // compare by ref

        std::string fmt(const char* tab = "    ") const;


    private:
        friend class yama::internal::env_instance;
        struct _data_t final {
            std::unordered_map<str, parcel_id> name_to_id;
            std::unordered_map<parcel_id, str> id_to_name;
        };
        // wrap in a res so we can cheaply copy env objects around w/out issue
        res<_data_t> _data;
    };
}

YAMA_SETUP_FORMAT(yama::internal::env, x.fmt());

namespace yama::internal {


    // env_instance is used by owner of env to mutate its state

    class env_instance final {
    public:
        env_instance() = default;
        env_instance(const env_instance&) = delete;
        env_instance(env_instance&&) noexcept = default;

        ~env_instance() noexcept = default;

        env_instance& operator=(const env_instance&) = delete;
        env_instance& operator=(env_instance&&) noexcept = default;


        const internal::env& get() const noexcept;

        void add(const str& name, parcel_id id);


    private:
        internal::env _env;
    };
}

