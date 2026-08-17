

#pragma once


#include "../yama/yama.h"


namespace _ym {


	// Encapsulates a RAII obj ref which is moving through the backend.
	class TempRef final {
	public:
		TempRef() = default;
		// Does not secure a new ref.
		TempRef(YmObj* ref, YmRefPolicy policy, bool frontend) noexcept;
		TempRef(std::nullptr_t) noexcept;
		~TempRef() noexcept;
		TempRef(const TempRef&) = delete;
		TempRef(TempRef&& other) noexcept;
		TempRef& operator=(const TempRef&) = delete;
		TempRef& operator=(TempRef&& other) noexcept;

		static TempRef borrow(YmObj* ref) noexcept;
		static TempRef take(YmObj* ref, bool frontend, bool secure) noexcept;
		static TempRef takeIfOk(YmObj* ref, bool frontend, bool secure) noexcept;
		// Conflates YM_TAKE and YM_TAKE_IF_OK.
		static TempRef borrowOrTake(YmObj* ref, bool frontend, bool secureIfTaken, YmRefPolicy policy) noexcept;


		explicit operator bool() const noexcept;

		YmObj* get() const noexcept;
		operator YmObj* () const noexcept;

		YmObj& operator*() const noexcept;
		YmObj* operator->() const noexcept;

		YmRefPolicy policy() const noexcept;

		// Doesn't release if YM_TAKE_IF_OK.
		void drop() noexcept;
		// Drops the ref such that if YM_TAKE_IF_OK, the drop is considered NOT to be
		// done 'in error', and thus the YM_TAKE_IF_OK ref is to be released.
		void dropAsOk() noexcept;
		// Disassociate *this from its ref ptr, returning the ref ptr, and resetting
		// *this, w/out performing dropping.
		YmObj* consume() noexcept;

		// TODO: Find a better name for this.

		// Changes frontend/backend of *this, returning *this.
		TempRef& changeSide(bool frontend) noexcept;


	private:
		YmObj* _ref = nullptr;
		YmRefPolicy _policy = YM_BORROW;
		bool _frontend = false;
	};

	// A version of InternalRef which specifically has a trivial dtor, meaning
	// that it can be used in untagged unions.
	// The catch w/ LiteInternalRef is the the dtor, being trivial, DOES NOT
	// call 'drop', meaning the end-user of the class has to do that MANUALLY.
	// This also extends to move-assign which will NOT CALL DROP FOR THE OLD VALUE.
	class LiteInternalRef final {
	public:
		LiteInternalRef() = default;
		LiteInternalRef(TempRef value) noexcept;
		LiteInternalRef(std::nullptr_t) noexcept;
		// Does NOT call drop.
		~LiteInternalRef() noexcept = default;
		LiteInternalRef(const LiteInternalRef&) = delete;
		LiteInternalRef(LiteInternalRef&&) noexcept = default;
		LiteInternalRef& operator=(const LiteInternalRef&) = delete;
		// Does NOT call drop (for old value.)
		// Avoid using this, use LiteInternalRef::assign instead.
		LiteInternalRef& operator=(LiteInternalRef&&) noexcept = default;


		explicit operator bool() const noexcept;

		YmObj* get() const noexcept;
		operator YmObj* () const noexcept;

		YmObj& operator*() const noexcept;
		YmObj* operator->() const noexcept;

		TempRef borrow() const noexcept;
		// Takes a copy of this ref (ie. w/out dropping it.)
		TempRef take(bool frontend) const noexcept;
		// if shouldBorrow == false, takes a copy of this ref (ie. w/out dropping it.)
		TempRef borrowOrTake(bool shouldBorrow, bool frontend) const noexcept;
		// Steals this ref (ie. w/ dropping it.)
		TempRef steal(bool frontend) noexcept;
		void drop() noexcept;

		// NOTE: Can't delete move-assign above as we need a trivial one to allow for
		//		 LiteInternalRef to work in untagged unions (which is why this class
		//		 even exists at all.)

		// Performs move-assign such that drop is called for old value.
		// Prefer this over normal move-assign for LiteInternalRef.
		void assign(TempRef other) noexcept;


	private:
		YmObj* _ref = nullptr;
	};

	// Encapsulates RAII storage of an internal obj ref.
	class InternalRef final {
	public:
		InternalRef() = default;
		InternalRef(TempRef value) noexcept;
		InternalRef(std::nullptr_t) noexcept;
		// Automatically calls drop.
		~InternalRef() noexcept;
		InternalRef(const InternalRef&) = delete;
		InternalRef(InternalRef&& other) noexcept;
		InternalRef& operator=(const InternalRef&) = delete;
		InternalRef& operator=(InternalRef&& other) noexcept;


		explicit operator bool() const noexcept;

		YmObj* get() const noexcept;
		operator YmObj* () const noexcept;

		YmObj& operator*() const noexcept;
		YmObj* operator->() const noexcept;

		TempRef borrow() const noexcept;
		// Takes a copy of this ref (ie. w/out dropping it.)
		TempRef take(bool frontend) const noexcept;
		// if shouldBorrow == false, takes a copy of this ref (ie. w/out dropping it.)
		TempRef borrowOrTake(bool shouldBorrow, bool frontend) const noexcept;
		// Steals this ref (ie. w/ dropping it.)
		TempRef steal(bool frontend) noexcept;
		void drop() noexcept;


	private:
		LiteInternalRef _ref;
	};
}

