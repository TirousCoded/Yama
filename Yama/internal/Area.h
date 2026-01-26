

#pragma once


#include "Resource.h"
#include "Section.h"
#include "YmItem.h"
#include "YmParcel.h"

#include "../yama++/meta.h"


namespace _ym {


	class Area final {
	public:
		Section<YmItem> items;
		Section<YmParcel> parcels;


		Area() = default;


		// Sets the upstream area, or no upstream area if upstream == nullptr.
		inline void setUpstream(Area* upstream) noexcept {
			items.setUpstream(upstream ? &upstream->items : nullptr);
			parcels.setUpstream(upstream ? &upstream->parcels : nullptr);
		}

		// Discards all resources in the area.
		inline void discard(bool propagateUpstream = false) noexcept {
			items.discard(propagateUpstream);
			parcels.discard(propagateUpstream);
		}

        // Transfers the resources from the area to the upstream area.
        // Fails quietly if there is no upstream area.
        // Behaviour is undefined if a name collision occurs between this area and upstream.
        inline void commit() {
			items.commit();
			parcels.commit();
        }

        inline void commitOrDiscard(bool commit) {
			items.commitOrDiscard(commit);
			parcels.commitOrDiscard(commit);
        }
		// lockIfCommit is locked via std::scoped_lock.
		inline void commitOrDiscard(bool commit, ym::Lockable auto& lockIfCommit) {
			if (commit) {
				std::scoped_lock lk(lockIfCommit);
				this->commit();
			}
			else {
				discard();
			}
		}
	};
}

