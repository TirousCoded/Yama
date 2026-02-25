

#pragma once


#include <optional>
#include <queue>

#include "TermStk.h"


namespace _ym {


    // NOTE: Loading occurs via the following:
    //          1) An input specifier is provided by end-user w/ which to load/import an type/parcel,
    //             recursively (though not processed immediately) loading dependencies if the former.
    //          2) The input specifier is parsed and is then interpreted.
    //          3) A stack machine of 'terms' is used during interpretation to compute terms encapsulating
    //             types/parcels, loading/importing incrementally as a function of stack machine operations.
    //          4) When a stack machine operation loads new type data, the following occur:
    //              a) New type data is generated.
    //              b) If a non-member, then type data is generated for each of its members.
    //              c) Performs early resolution of its constant table (this also occurs for members.)
    //              d) Adds the new type (and its members) to the late resolution queue.
    //          5) Each type in the late resolution queue are processed, performing the following for each:
    //              a) Resolves ref consts that weren't early resolvable.
    //              b) This resolution involves a version of the loading sequence described above, but w/
    //                 additional environment metadata (defining things like %here, $Self, etc.)
    //              c) This resolution may likewise add new types to the late resolution queue, which will
    //                 in turn be processed, w/ this processing occurring until the queue is empty.
    //          6) Each type newly loaded undergoes constraint checking for each of their type arguments.

    // NOTE: Above 'resolution' describes the process of resolving constant table entries for newly
    //       loaded types, w/ for ref consts this entailing their parsing/interpreting.
    //
    //       Resolution is split up into two phases: 'early' and 'late':
    //          - Early resolution covers value constants, and ref consts of the forms '$Self' and
    //            '$Self::[MEMBER]' which refer to a types owner/members.
    //          - Late resolution covers ref consts not resolved during early resolution.
    //
    //       Early resolution is needed so that types can have their owner/member types queried, which is
    //       needed for certain stack machine operations to be able to execute properly, and so these need
    //       to be done up-front upon initial type data generation.
    //
    //       Originally, all resolution was done up-front like early resolution is, w/ us having a stack
    //       of 'scopes', each w/ their own isolated term stack, w/ generation/resolution of new type data
    //       occurring recursively, traversing the type dependency graph in a depth-first manner.
    //
    //       This was changed so that late resolution occurs after 100% finishing interpreting of the
    //       specifier which originally generated the type, as doing this removed the recursive 'nesting'
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
		YmType* load(const Spec& fullname);


	private:
		TermStk _termStk;
		std::queue<ym::Safe<YmType>> _lateResolveQueue;
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

        struct _GenTypeDataResult final {
            ym::Safe<YmType> type; // Type either newly init, or existing one that was fetched.
            bool original; // If type was newly init, and thus in need of further setup.
        };
        // This does not perform resolution.
        _GenTypeDataResult _genTypeData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::TypeInfo> info,
            YmType* owner,
            std::vector<ym::Safe<YmType>> typeArgs = {});
        // All type data generation originates from non-members, which in turn generate for members.
        ym::Safe<YmType> _genNonMemberTypeData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::TypeInfo> info,
            std::vector<ym::Safe<YmType>> typeArgs = {});
        // Call this prior to early resolution so this info is available for it.
        void _genTypeDataForMembers(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::TypeInfo> info,
            ym::Safe<YmType> self);
        void _genMemberTypeData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::TypeInfo> info,
            ym::Safe<YmType> self);

        const _ym::TypeInfo* _lookupMemberInfo(
            ym::Safe<YmParcel> parcel,
            const _ym::TypeInfo& ownerInfo,
            const std::string& memberName) const;
        static std::string _localNameOfMember(
            const _ym::TypeInfo& ownerInfo,
            const std::string& memberName);

        bool _isEarlyResolveConst(const ConstInfo& constInfo) const;
        void _earlyResolveType(YmType& x, ym::Safe<YmType> self);
        void _earlyResolveConsts(YmType& x, ym::Safe<YmType> self);
        void _scheduleLateResolve(YmType& x);
        void _processLateResolveQueue();
        void _lateResolveType(YmType& x);
        void _lateResolveConsts(YmType& x);
        void _lateResolveRefConst(YmType& x, ConstIndex index);
        // Flushes remaining data from late resolve queue (ie. if load exits due to error.)
        void _flushLateResolveQueue() noexcept;

        YmParcel* _initialImport(const Spec& path);
        YmType* _initialLoad(const Spec& fullname);

        void _checkConstraintTypeLegality();
        void _enforceConstraints();
        void _checkRefConstCallSigConformance();
	};
}

