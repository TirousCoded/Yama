

#pragma once


#include <string>
#include <map>

#include "Spec.h"


namespace _ym {


	// NOTE: The usage of std::map here is due to how it will ensure that our map keys are sorted in
	//		 ascending order. This is important for both RedirectSet::resolve and Redirects::compute,
	//		 as it makes it easy to impl all redirect shadowing semantics.

	class RedirectSet final {
	public:
		RedirectSet() = default;
		~RedirectSet() noexcept = default;
		RedirectSet(const RedirectSet&) = default;
		RedirectSet(RedirectSet&&) noexcept = default;
		RedirectSet& operator=(const RedirectSet&) = default;
		RedirectSet& operator=(RedirectSet&&) noexcept = default;


		// Returns path modified by the appropriate redirect, or unmodified is no redirect for it was found.
		Spec resolve(const Spec& path) const;


	private:
		friend class Redirects;


		// Maps 'before' paths to 'after' paths.
		std::map<Spec, Spec> _redirects;
	};

	class Redirects final {
	public:
		Redirects() = default;


		void add(const Spec& subject, const Spec& before, const Spec& after);

		// Computes the redirect set for path.
		RedirectSet compute(const Spec& path) const;


	private:
		// Maps pair of 'subject' and 'before' paths to 'after' paths.
		std::map<std::pair<Spec, Spec>, Spec> _redirects;
	};
}

