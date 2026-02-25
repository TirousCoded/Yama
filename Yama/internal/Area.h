

#pragma once


#include "Resource.h"
#include "Section.h"
#include "YmType.h"
#include "YmParcel.h"

#include "../yama++/meta.h"


namespace _ym {


	class Area final {
	public:
		Section<YmType> types;
		Section<YmParcel> parcels;


		Area() = default;


		// Sets the upstream area, or no upstream area if upstream == nullptr.
		inline void setUpstream(Area* upstream) noexcept {
			types.setUpstream(upstream ? &upstream->types : nullptr);
			parcels.setUpstream(upstream ? &upstream->parcels : nullptr);
		}

		// Discards all resources in the area.
		inline void discard(bool propagateUpstream = false) noexcept {
			types.discard(propagateUpstream);
			parcels.discard(propagateUpstream);
		}

        // Transfers the resources from the area to the upstream area.
        // Fails quietly if there is no upstream area.
        // Behaviour is undefined if a name collision occurs between this area and upstream.
        inline void commit() {
			types.commit();
			parcels.commit();
        }

        inline void commitOrDiscard(bool commit) {
			types.commitOrDiscard(commit);
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

