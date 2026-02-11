

#pragma once


#include <string>
#include <map>


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
		std::string resolve(std::string path) const;


	private:
		friend class Redirects;


		// Maps 'before' paths to 'after' paths.
		std::map<std::string, std::string> _redirects;
	};

	class Redirects final {
	public:
		Redirects() = default;


		void add(const std::string& subject, const std::string& before, const std::string& after);

		// Computes the redirect set for path.
		RedirectSet compute(const std::string& path) const;


	private:
		// Maps pair of 'subject' and 'before' paths to 'after' paths.
		std::map<std::pair<std::string, std::string>, std::string> _redirects;
	};
}

