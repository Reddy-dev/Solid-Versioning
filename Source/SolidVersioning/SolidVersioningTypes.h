// Elie Wiese-Namir © 2025. All Rights Reserved.

#pragma once


#include <functional>

#include "SolidMacros.h"
#include "Standard/robin_hood.h"
#include "Types/SolidNotNull.h"

#define UE_API SOLIDVERSIONING_API

#define START_SOLID_VERSION(Name) \
	struct F##Name##CustomVersion \
	{ \
		static const FGuid GUID; \
		\
		enum Type \
		{ \
			/** Before any version changes were made */ \
			BeforeCustomVersionWasAdded = 0,

#define DEFINE_SOLID_VERSION(VersionName) \
			VersionName,

#define END_SOLID_VERSION(Name) \
			\
			/** -----<new versions can be added above this line>----- **/ \
			VersionPlusOne, \
			LatestVersion = VersionPlusOne - 1 \
		}; \
	};

#define IMPLEMENT_SOLID_VERSION(Name, Guid) \
	const FGuid F##Name##CustomVersion::GUID(##Guid); \
	static FRegisterSolidCustomVersion Register##Name##CustomVersion(F##Name##CustomVersion::GUID, F##Name##CustomVersion::LatestVersion, TEXT(#Name));

namespace Solid
{
	class UE_API FAssetMigrationRegistry
	{
	public:
		using FStepFunctionType = std::function<void(const TSolidNotNull<UObject*>)>;

		struct FAssetMigrationStep
		{
			int32 ToVersion;
			FStepFunctionType StepFunction;
		};

		static FAssetMigrationRegistry& Get()
		{
			static FAssetMigrationRegistry GAssetMigrationRegistry;
			return GAssetMigrationRegistry;
		}

		void Register(const TSolidNotNull<UClass*> ForClass, int32 ToVersion, FStepFunctionType Func)
		{
			TArray<FAssetMigrationStep>& Array = Steps.FindOrAdd(ForClass);
			Array.Add({ ToVersion, MoveTemp(Func) });
			
			Array.Sort([](const FAssetMigrationStep& Left, const FAssetMigrationStep& Right)
			{
				return Left.ToVersion < Right.ToVersion;
			});
		}

		bool Migrate(const TSolidNotNull<UObject*> Object, const int32 FromVersion, const int32 ToVersion) const  // NOLINT(modernize-use-nodiscard)
		{
			bool bChanged = false;
			
			if (const TArray<FAssetMigrationStep>* Array = Steps.Find(Object->GetClass()))
			{
				for (const FAssetMigrationStep& Step : *Array)
				{
					if (Step.ToVersion > FromVersion && Step.ToVersion <= ToVersion)
					{
						Step.StepFunction(Object);
						bChanged = true;
					}
				}
			}
			
			return bChanged;
		}

	private:
		TMap<UClass*, TArray<FAssetMigrationStep>> Steps;
		
	}; // class FAssetMigrationRegistry

	NO_DISCARD static bool FPropertyMatchesCDO(const TSolidNotNull<const UObject*> Object, const TSolidNotNull<const FProperty*> Property);
	NO_DISCARD static bool PropertyMatchesCDO(const TSolidNotNull<const UObject*> Object, const FName PropertyName);
	
} // namespace Solid

#define REGISTER_ASSET_MIGRATION_STEP(ClassType, ToVersion, FunctionBody) \
	namespace \
	{ \
		struct FAutoRegister##ClassType##ToVersion \
		{ \
			FAutoRegister##ClassType##ToVersion() \
			{ \
				Solid::FAssetMigrationRegistry::Get().Register(ClassType::StaticClass(), ToVersion, \
					[](const TSolidNotNull<UObject*> Object); \
					{ \
						const TSolidNotNull<AssetClass*> Self = CastChecked<AssetClass>(Object); \
						FunctionBody \
					}); \
			} \
		}; \
		static FAutoRegister##ClassType##ToVersion AutoRegister##ClassType##ToVersionInstance; \
	}

#undef UE_API // SOLIDVERSIONING_API