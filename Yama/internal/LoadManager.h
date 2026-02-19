

#pragma once


#include <optional>
#include <queue>

#include "TermStk.h"


namespace _ym {


    // NOTE: Loading occurs via the following:
    //          1) An input specifier is provided by end-user w/ which to load/import an item/parcel,
    //             recursively (though not processed immediately) loading dependencies if the former.
    //          2) The input specifier is parsed and is then interpreted.
    //          3) A stack machine of 'terms' is used during interpretation to compute terms encapsulating
    //             items/parcels, loading/importing incrementally as a function of stack machine operations.
    //          4) When a stack machine operation loads new item data, the following occur:
    //              a) New item data is generated.
    //              b) If a non-member, then item data is generated for each of its members.
    //              c) Performs early resolution of its constant table (this also occurs for members.)
    //              d) Adds the new item (and its members) to the late resolution queue.
    //          5) Each item in the late resolution queue are processed, performing the following for each:
    //              a) Resolves ref consts that weren't early resolvable.
    //              b) This resolution involves a version of the loading sequence described above, but w/
    //                 additional environment metadata (defining things like %here, $Self, etc.)
    //              c) This resolution may likewise add new items to the late resolution queue, which will
    //                 in turn be processed, w/ this processing occurring until the queue is empty.
    //          6) Each item newly loaded undergoes constraint checking for each of their item arguments.

    // NOTE: Above 'resolution' describes the process of resolving constant table entries for newly
    //       loaded items, w/ for ref consts this entailing their parsing/interpreting.
    //
    //       Resolution is split up into two phases: 'early' and 'late':
    //          - Early resolution covers value constants, and ref consts of the forms '$Self' and
    //            '$Self::[MEMBER]' which refer to an items owner/members.
    //          - Late resolution covers ref consts not resolved during early resolution.
    //
    //       Early resolution is needed so that items can have their owner/member items queried, which is
    //       needed for certain stack machine operations to be able to execute properly, and so these need
    //       to be done up-front upon initial item data generation.
    //
    //       Originally, all resolution was done up-front like early resolution is, w/ us having a stack
    //       of 'scopes', each w/ their own isolated term stack, w/ generation/resolution of new item data
    //       occurring recursively, traversing the item dependency graph in a depth-first manner.
    //
    //       This was changed so that late resolution occurs after 100% finishing interpreting of the
    //       specifier which originally generated the item, as doing this removed the recursive 'nesting'
    //       of loading events, which was making debugging/maintaining loading system code harder due to
    //       the amount of complexity it added to debug outputs. This change is also intended to make
    //       overall loading system code easier to deal w/.

	class LoadManager final {
	public:
		const ym::Safe<Area> staging;


		LoadManager(
			Area& staging,
			PathBindings& binds,
			Redirects& redirects);


		// NOTE: Methods exposed for end-user of LoadManager, w/ these handling
		//		 entire import/load process.

		YmParcel* import(const Spec& path);
		YmItem* load(const Spec& fullname);


	private:
		TermStk _termStk;
		std::queue<ym::Safe<YmItem>> _lateResolveQueue;
        bool _failFlag = false;


        void _beginImportOrLoad();
        void _endImportOrLoad();

        bool _good() const noexcept;
        void _fail() noexcept;
        void _clearFlag() noexcept;

        template<typename... Args>
        inline void _err(
            YmErrCode code,
            std::format_string<Args...> fmt,
            Args&&... args) {
            _ym::Global::raiseErr(code, fmt, std::forward<Args>(args)...);
            _fail(); // Don't forget!
        }

        struct _GenItemDataResult final {
            ym::Safe<YmItem> item; // Item either newly init, or existing one that was fetched.
            bool original; // If item was newly init, and thus in need of further setup.
        };
        // This does not perform resolution.
        _GenItemDataResult _genItemData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::ItemInfo> info,
            YmItem* owner,
            std::vector<ym::Safe<YmItem>> itemArgs = {});
        // All item data generation originates from non-members, which in turn generate for members.
        ym::Safe<YmItem> _genNonMemberItemData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::ItemInfo> info,
            std::vector<ym::Safe<YmItem>> itemArgs = {});
        // Call this prior to early resolution so this info is available for it.
        void _genItemDataForMembers(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::ItemInfo> info,
            ym::Safe<YmItem> self);
        void _genMemberItemData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::ItemInfo> info,
            ym::Safe<YmItem> self);

        const _ym::ItemInfo* _lookupMemberInfo(
            ym::Safe<YmParcel> parcel,
            const _ym::ItemInfo& ownerInfo,
            const std::string& memberName) const;
        static std::string _localNameOfMember(
            const _ym::ItemInfo& ownerInfo,
            const std::string& memberName);

        bool _isEarlyResolveConst(const ConstInfo& constInfo) const;
        void _earlyResolveItem(YmItem& x, ym::Safe<YmItem> self);
        void _earlyResolveConsts(YmItem& x, ym::Safe<YmItem> self);
        void _scheduleLateResolve(YmItem& x);
        void _processLateResolveQueue();
        void _lateResolveItem(YmItem& x);
        void _lateResolveConsts(YmItem& x);
        void _lateResolveRefConst(YmItem& x, ConstIndex index);
        // Flushes remaining data from late resolve queue (ie. if load exits due to error.)
        void _flushLateResolveQueue() noexcept;

        YmParcel* _initialImport(const Spec& path);
        YmItem* _initialLoad(const Spec& fullname);

        void _checkConstraintTypeLegality();
        void _enforceConstraints();
        void _checkRefConstCallSigConformance();
	};
}

