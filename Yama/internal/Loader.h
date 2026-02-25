

#pragma once


#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>

#include "../yama/yama.h"
#include "Area.h"
#include "general.h"
#include "LoadManager.h"
#include "PathBindings.h"
#include "Redirects.h"
#include "YmType.h"
#include "YmParcel.h"


namespace _ym {


    // Base class of all loaders.
    class Loader {
    public:
        Loader() = default;
        virtual ~Loader() noexcept = default;


        // Tells the loader to reset its state.
        // What exactly occurs is implementation specific.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual void reset() noexcept = 0;

        // Acquires parcel w/out attempting to import.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmParcel> fetchParcel(const Spec& path) const noexcept = 0;

        // Acquires type w/out attempting to load.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmType> fetchType(const Spec& fullname, bool* failedDueToCallSigNonConform = nullptr) const noexcept = 0;

        // Acquires parcel, attempting import if necessary.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmParcel> import(const Spec& path) = 0;

        // Acquires type, attempting load if necessary.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmType> load(const Spec& fullname) = 0;
    };

    // Base class of all unsynchronized loader.
    class UnsynchronizedLoader : public Loader {
    public:
        UnsynchronizedLoader() = default;


        // Returns an area containing all fully imported/loaded parcels/types.
        virtual const Area& commits() const = 0;
    };

    // Base class of all synchronized loader.
    class SynchronizedLoader : public Loader {
    public:
        SynchronizedLoader() = default;
    };

    // Thread-safe loader used by domains, servicing downstream contexts loaders.
    class DmLoader final : public SynchronizedLoader {
    public:
        DmLoader();


        bool bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef, bool bindIsForYamaParcel = false);
        bool addRedirect(const std::string& subject, const std::string& before, const std::string& after);

        void reset() noexcept override;
        std::shared_ptr<YmParcel> fetchParcel(const Spec& path) const noexcept override;
        std::shared_ptr<YmType> fetchType(const Spec& fullname, bool* failedDueToCallSigNonConform = nullptr) const noexcept override;
        std::shared_ptr<YmParcel> import(const Spec& path) override;
        std::shared_ptr<YmType> load(const Spec& fullname) override;


    private:
        PathBindings _binds;
        Redirects _redirects;
        Area _commits, _staging;
        LoadManager _ldr;

        // NOTE: _updateLock protects _binds/_redirects as their data is used during loading.

        mutable std::shared_mutex _accessLock; // Protects _commits.
        mutable std::mutex _updateLock; // Protects _staging/_binds/_redirects/_ldrState.


        void _bindYamaParcel();
    };

    // Thread-unsafe loader used by contexts, existing downstream of domain loaders.
    class CtxLoader final : public UnsynchronizedLoader {
    public:
        CtxLoader(const std::shared_ptr<Loader>& upstream);


        std::shared_ptr<Loader> upstream() const;

        void reset() noexcept override;
        std::shared_ptr<YmParcel> fetchParcel(const Spec& path) const noexcept override;
        std::shared_ptr<YmType> fetchType(const Spec& fullname, bool* failedDueToCallSigNonConform = nullptr) const noexcept override;
        std::shared_ptr<YmParcel> import(const Spec& path) override;
        std::shared_ptr<YmType> load(const Spec& fullname) override;
        const Area& commits() const override;


    private:
        std::weak_ptr<Loader> _upstream;
        Area _commits;
    };
}

