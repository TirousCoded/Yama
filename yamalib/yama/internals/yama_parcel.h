

#pragma once


#include "../core/parcel.h"


namespace yama::internal {


    class yama_parcel final : public yama::parcel {
    public:
        yama_parcel() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ str::lit("yama"), {} };
            return *md;
        }
        std::optional<import_result> import(const str& relative_path) override final;


    private:
        std::optional<yama::parcel_metadata> md;
        std::shared_ptr<module> _root_modinf, _util_modinf;


        res<module> _get_root_modinf();
        res<module> _get_util_modinf();
    };
}

