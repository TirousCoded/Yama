

#pragma once


#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>

#include "../yama/yama.h"
#include "Area.h"
#include "YmItem.h"
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
        virtual std::shared_ptr<YmParcel> fetchParcel(const std::string& path) const noexcept = 0;
        // Acquires parcel w/out attempting to import.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmParcel> fetchParcel(YmPID pid) const noexcept = 0;

        // Acquires item w/out attempting to load.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmItem> fetchItem(const std::string& fullname) const noexcept = 0;
        // Acquires item w/out attempting to load.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmItem> fetchItem(YmGID gid) const noexcept = 0;

        // Acquires parcel, attempting import if necessary.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmParcel> import(const std::string& path) = 0;
        // Acquires parcel, attempting import if necessary.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmParcel> import(YmPID pid) = 0;

        // Acquires item, attempting load if necessary.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmItem> load(const std::string& fullname) = 0;
        // Acquires item, attempting load if necessary.
        // In synchronized loaders this is guaranteed to be thread-safe.
        virtual std::shared_ptr<YmItem> load(YmGID gid) = 0;
    };

    // Base class of all unsynchronized loader.
    class UnsynchronizedLoader : public Loader {
    public:
        UnsynchronizedLoader() = default;


        // Returns the area containing all fully imported parcels.
        virtual const Area<YmParcel>& parcels() const = 0;

        // Returns the area containing all fully loaded items.
        virtual const Area<YmItem>& items() const = 0;
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


        bool bindParcelDef(const std::string& path, ym::Safe<YmParcelDef> parceldef);

        void reset() noexcept override;
        std::shared_ptr<YmParcel> fetchParcel(const std::string& path) const noexcept override;
        std::shared_ptr<YmParcel> fetchParcel(YmPID pid) const noexcept override;
        std::shared_ptr<YmItem> fetchItem(const std::string& fullname) const noexcept override;
        std::shared_ptr<YmItem> fetchItem(YmGID gid) const noexcept override;
        std::shared_ptr<YmParcel> import(const std::string& path) override;
        std::shared_ptr<YmParcel> import(YmPID pid) override;
        std::shared_ptr<YmItem> load(const std::string& fullname) override;
        std::shared_ptr<YmItem> load(YmGID gid) override;


    private:
        // The '_pub***' areas are for resources that have been successfully
        // loaded and are available for end-user query.
        //
        // Need _accessLock inclusive/exclusive lock to access.
        // Need _accessLock exclusive lock to modify.

        // The '_priv***' areas are for resources which are in the process of
        // loading, but are not yet fully loaded and are not available for
        // end-user query.
        //
        // Need _sessionLock lock to access/modify.

        // Loading (which here is conflated w/ importing) operates via 'sessions'
        // where resources are loaded in bulk, resolving dependency relationships.
        // 
        // At the end of a loading session, the resources are in the private areas,
        // and are either promoted to public areas, or are instead discarded, based
        // on if loading succeeded or not.
        //
        // Sessions also encapsulate things like parcel bindings.
        //      * Methods like _[begin|end]Session needn't always be used when working
        //        w/ bindings protected by _sessionLock.


        _ym::Area<YmParcel> _pubParcels, _privParcels, _bindings;
        _ym::Area<YmItem> _pubItems, _privItems;
        bool _success = false;
        mutable std::shared_mutex _accessLock; // Protects access to _pub*** area state.
        mutable std::mutex _sessionLock; // Protects access to session related state.


        // These methods are unsynchronized.

        void _beginSession();
        void _endSession();
        std::shared_ptr<YmParcel> _import(const std::string& path);
        std::shared_ptr<YmParcel> _import(YmPID pid);
        std::shared_ptr<YmItem> _load(const std::string& fullname);
        std::shared_ptr<YmItem> _load(YmGID gid);
        void _resolveConsts(YmItem& x);
    };

    // Thread-unsafe loader used by contexts, existing downstream of domain loaders.
    class CtxLoader final : public UnsynchronizedLoader {
    public:
        CtxLoader(const std::shared_ptr<Loader>& upstream);


        std::shared_ptr<Loader> upstream() const;

        void reset() noexcept override;
        std::shared_ptr<YmParcel> fetchParcel(const std::string& path) const noexcept override;
        std::shared_ptr<YmParcel> fetchParcel(YmPID pid) const noexcept override;
        std::shared_ptr<YmItem> fetchItem(const std::string& fullname) const noexcept override;
        std::shared_ptr<YmItem> fetchItem(YmGID gid) const noexcept override;
        std::shared_ptr<YmParcel> import(const std::string& path) override;
        std::shared_ptr<YmParcel> import(YmPID pid) override;
        std::shared_ptr<YmItem> load(const std::string& fullname) override;
        std::shared_ptr<YmItem> load(YmGID gid) override;
        const Area<YmParcel>& parcels() const override;
        const Area<YmItem>& items() const override;


    private:
        std::weak_ptr<Loader> _upstream;
        Area<YmParcel> _parcels;
        Area<YmItem> _items;
    };
}

