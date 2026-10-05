#include "ObjectModifications.h"

#include <any>
#include <regex>
#include <unordered_set>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace ObjectModifications
{
	namespace
	{
		constexpr std::string_view kTypeName = "ObjectModification";

		enum class FilterType
		{
			kFormID
		};

		std::string_view FilterTypeToString(FilterType a_value)
		{
			switch (a_value)
			{
			case FilterType::kFormID:
				return "FilterByFormID";
			default:
				return std::string_view{};
			}
		}

		enum class ElementType
		{
			kIncludes,
			kProperties,
		};

		std::string_view ElementTypeToString(ElementType a_value)
		{
			switch (a_value)
			{
			case ElementType::kIncludes:
				return "Includes";
			case ElementType::kProperties:
				return "Properties";
			default:
				return std::string_view{};
			}
		}

		enum class OperationType
		{
			kClear,
			kAdd,
			kDelete,
		};

		std::string_view OperationTypeToString(OperationType a_value)
		{
			switch (a_value)
			{
			case OperationType::kClear:
				return "Clear";
			case OperationType::kAdd:
				return "Add";
			case OperationType::kDelete:
				return "Delete";
			default:
				return std::string_view{};
			}
		}

		struct ConfigData
		{
			struct Operation
			{
				struct IncludesData
				{
					std::string Form;
					std::uint8_t MinLevel = 0;
					bool Optional = false;
					bool DontUseAll = false;
				};

				struct PropertiesData
				{
					std::string ValueType;
					std::string FunctionType;
					std::string Property;
					std::any Value1;
					std::any Value2;
				};

				OperationType OpType;
				std::optional<std::any> OpData;
			};

			FilterType Filter;
			std::string FilterForm;
			ElementType Element;
			std::vector<Operation> Operations;
		};

		using IncludeContainer = std::array<std::byte, sizeof(RE::BGSMod::Attachment::Instance)>;
		using PropertyContainer = std::array<std::byte, sizeof(RE::BGSMod::Property::Mod)>;

		struct PatchData
		{
			struct IncludesData
			{
				bool Clear = false;
				std::vector<IncludeContainer> Add;
				std::vector<IncludeContainer> Delete;
			};

			struct PropertiesData
			{
				bool Clear = false;
				std::vector<PropertyContainer> Add;
				std::vector<PropertyContainer> Delete;
			};

			std::optional<IncludesData> Includes;
			std::optional<PropertiesData> Properties;
		};

		std::vector<Parsers::Statement<ConfigData>> g_configVec;
		std::unordered_map<RE::BGSMod::Attachment::Mod*, PatchData> g_patchMap;

		const std::unordered_set<std::string_view> g_propertySet = {
			"Enchantments",
			"BashImpactDataSet",
			"BlockMaterial",
			"Keywords",
			"Weight",
			"Value",
			"Rating",
			"AddonIndex",
			"BodyPart",
			"DamageTypeValue",
			"ActorValues",
			"Health",
			"ColorRemappingIndex",
			"MaterialSwaps",
			"ForcedInventory",
			"XPOffset",
			"Speed",
			"Reach",
			"MinRange",
			"MaxRange",
			"AttackDelaySec",
			"Unknown 5",
			"OutOfRangeDamageMult",
			"SecondaryDamage",
			"CriticalChargeBonus",
			"HitBehaviour",
			"Rank",
			"Unknown 11",
			"AmmoCapacity",
			"Unknown 13",
			"Unknown 14",
			"Type",
			"IsPlayerOnly",
			"NPCsUseAmmo",
			"HasChargingReload",
			"IsMinorCrime",
			"IsFixedRange",
			"HasEffectOnDeath",
			"HasAlternateRumble",
			"IsNonHostile",
			"IgnoreResist",
			"IsAutomatic",
			"CantDrop",
			"IsNonPlayable",
			"AttackDamage",
			"AimModel",
			"AimModelMinConeDegrees",
			"AimModelMaxConeDegrees",
			"AimModelConeIncreasePerShot",
			"AimModelConeDecreasePerSec",
			"AimModelConeDecreaseDelayMs",
			"AimModelConeSneakMultiplier",
			"AimModelRecoilDiminishSpringForce",
			"AimModelRecoilDiminishSightsMult",
			"AimModelRecoilMaxDegPerShot",
			"AimModelRecoilMinDegPerShot",
			"AimModelRecoilHipMult",
			"AimModelRecoilShotsForRunaway",
			"AimModelRecoilArcDeg",
			"AimModelRecoilArcRotateDeg",
			"AimModelConeIronSightsMultiplier",
			"HasScope",
			"ZoomDataFOVMult",
			"FireSeconds",
			"NumProjectiles",
			"AttackSound",
			"AttackSound2D",
			"AttackLoop",
			"AttackFailSound",
			"IdleSound",
			"EquipSound",
			"UnEquipSound",
			"SoundLevel",
			"ImpactDataSet",
			"Ammo",
			"CritEffect",
			"AimModelBaseStability",
			"ZoomData",
			"ZoomDataOverlay",
			"ZoomDataImageSpace",
			"ZoomDataCameraOffsetX",
			"ZoomDataCameraOffsetY",
			"ZoomDataCameraOffsetZ",
			"EquipSlot",
			"SoundLevelMult",
			"NPCAmmoList",
			"ReloadSpeed",
			"DamageTypeValues",
			"AccuracyBonus",
			"AttackActionPointCost",
			"OverrideProjectile",
			"HasBoltAction",
			"StaggerValue",
			"SightedTransitionSeconds",
			"FullPowerSeconds",
			"HoldInputToPower",
			"HasRepeatableSingleFire",
			"MinPowerPerShot",
			"CriticalDamageMult",
			"FastEquipSound",
			"DisableShells",
			"HasChargingAttack"
		};

		const std::unordered_map<std::string_view, std::uint32_t> g_armorPropertyMap = {
			{ "Enchantments", 0 },
			{ "BashImpactDataSet", 1 },
			{ "BlockMaterial", 2 },
			{ "Keywords", 3 },
			{ "Weight", 4 },
			{ "Value", 5 },
			{ "Rating", 6 },
			{ "AddonIndex", 7 },
			{ "BodyPart", 8 },
			{ "DamageTypeValue", 9 },
			{ "ActorValues", 10 },
			{ "Health", 11 },
			{ "ColorRemappingIndex", 12 },
			{ "MaterialSwaps", 13 }
		};

		const std::unordered_map<std::string_view, std::uint32_t> g_actorPropertyMap = {
			{ "Keywords", 0 },
			{ "ForcedInventory", 1 },
			{ "XPOffset", 2 },
			{ "Enchantments", 3 },
			{ "ColorRemappingIndex", 4 },
			{ "MaterialSwaps", 5 }
		};

		const std::unordered_map<std::string_view, std::uint32_t> g_weaponPropertyMap = {
			{ "Speed", 0 },
			{ "Reach", 1 },
			{ "MinRange", 2 },
			{ "MaxRange", 3 },
			{ "AttackDelaySec", 4 },
			{ "Unknown 5", 5 },
			{ "OutOfRangeDamageMult", 6 },
			{ "SecondaryDamage", 7 },
			{ "CriticalChargeBonus", 8 },
			{ "HitBehaviour", 9 },
			{ "Rank", 10 },
			{ "Unknown 11", 11 },
			{ "AmmoCapacity", 12 },
			{ "Unknown 13", 13 },
			{ "Unknown 14", 14 },
			{ "Type", 15 },
			{ "IsPlayerOnly", 16 },
			{ "NPCsUseAmmo", 17 },
			{ "HasChargingReload", 18 },
			{ "IsMinorCrime", 19 },
			{ "IsFixedRange", 20 },
			{ "HasEffectOnDeath", 21 },
			{ "HasAlternateRumble", 22 },
			{ "IsNonHostile", 23 },
			{ "IgnoreResist", 24 },
			{ "IsAutomatic", 25 },
			{ "CantDrop", 26 },
			{ "IsNonPlayable", 27 },
			{ "AttackDamage", 28 },
			{ "Value", 29 },
			{ "Weight", 30 },
			{ "Keywords", 31 },
			{ "AimModel", 32 },
			{ "AimModelMinConeDegrees", 33 },
			{ "AimModelMaxConeDegrees", 34 },
			{ "AimModelConeIncreasePerShot", 35 },
			{ "AimModelConeDecreasePerSec", 36 },
			{ "AimModelConeDecreaseDelayMs", 37 },
			{ "AimModelConeSneakMultiplier", 38 },
			{ "AimModelRecoilDiminishSpringForce", 39 },
			{ "AimModelRecoilDiminishSightsMult", 40 },
			{ "AimModelRecoilMaxDegPerShot", 41 },
			{ "AimModelRecoilMinDegPerShot", 42 },
			{ "AimModelRecoilHipMult", 43 },
			{ "AimModelRecoilShotsForRunaway", 44 },
			{ "AimModelRecoilArcDeg", 45 },
			{ "AimModelRecoilArcRotateDeg", 46 },
			{ "AimModelConeIronSightsMultiplier", 47 },
			{ "HasScope", 48 },
			{ "ZoomDataFOVMult", 49 },
			{ "FireSeconds", 50 },
			{ "NumProjectiles", 51 },
			{ "AttackSound", 52 },
			{ "AttackSound2D", 53 },
			{ "AttackLoop", 54 },
			{ "AttackFailSound", 55 },
			{ "IdleSound", 56 },
			{ "EquipSound", 57 },
			{ "UnEquipSound", 58 },
			{ "SoundLevel", 59 },
			{ "ImpactDataSet", 60 },
			{ "Ammo", 61 },
			{ "CritEffect", 62 },
			{ "BashImpactDataSet", 63 },
			{ "BlockMaterial", 64 },
			{ "Enchantments", 65 },
			{ "AimModelBaseStability", 66 },
			{ "ZoomData", 67 },
			{ "ZoomDataOverlay", 68 },
			{ "ZoomDataImageSpace", 69 },
			{ "ZoomDataCameraOffsetX", 70 },
			{ "ZoomDataCameraOffsetY", 71 },
			{ "ZoomDataCameraOffsetZ", 72 },
			{ "EquipSlot", 73 },
			{ "SoundLevelMult", 74 },
			{ "NPCAmmoList", 75 },
			{ "ReloadSpeed", 76 },
			{ "DamageTypeValues", 77 },
			{ "AccuracyBonus", 78 },
			{ "AttackActionPointCost", 79 },
			{ "OverrideProjectile", 80 },
			{ "HasBoltAction", 81 },
			{ "StaggerValue", 82 },
			{ "SightedTransitionSeconds", 83 },
			{ "FullPowerSeconds", 84 },
			{ "HoldInputToPower", 85 },
			{ "HasRepeatableSingleFire", 86 },
			{ "MinPowerPerShot", 87 },
			{ "ColorRemappingIndex", 88 },
			{ "MaterialSwaps", 89 },
			{ "CriticalDamageMult", 90 },
			{ "FastEquipSound", 91 },
			{ "DisableShells", 92 },
			{ "HasChargingAttack", 93 },
			{ "ActorValues", 94 }
		};

		class ObjectModificationParser : public Parsers::Parser<ConfigData>
		{
		public:
			ObjectModificationParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

		protected:
			std::optional<Parsers::Statement<ConfigData>> ParseExpressionStatement() override
			{
				ConfigData configData{};

				if (!ParseFilter(configData))
				{
					return std::nullopt;
				}

				auto token = reader.GetToken();
				if (token != ".")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				if (!ParseElement(configData))
				{
					return std::nullopt;
				}

				do
				{
					token = reader.GetToken();
					if (token != ".")
					{
						logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}

					if (!ParseOperation(configData))
					{
						return std::nullopt;
					}
				} while (reader.Peek() == ".");

				token = reader.GetToken();
				if (token != ";")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
			}

			void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override
			{
				auto indent = std::string(a_indent * 4, ' ');

				switch (a_configData.Element)
				{
				case ElementType::kIncludes:
				case ElementType::kProperties:
					logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
					for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); ++opIndex)
					{
						std::string opLog;

						switch (a_configData.Operations[opIndex].OpType)
						{
						case OperationType::kClear:
							opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
							break;

						case OperationType::kAdd:
						case OperationType::kDelete:
							if (a_configData.Element == ElementType::kIncludes)
							{
								const auto& op = a_configData.Operations[opIndex];
								const auto& opData = std::any_cast<const ConfigData::Operation::IncludesData&>(op.OpData.value());
								opLog = fmt::format(".{}({}, {}, {}, {})", OperationTypeToString(op.OpType), opData.Form, opData.MinLevel, opData.Optional, opData.DontUseAll);
							}
							else if (a_configData.Element == ElementType::kProperties)
							{
								const auto& opData = std::any_cast<const ConfigData::Operation::PropertiesData&>(a_configData.Operations[opIndex].OpData.value());
								if (opData.ValueType == "Int")
								{
									opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), opData.ValueType, opData.FunctionType, opData.Property, std::any_cast<std::uint32_t>(opData.Value1), std::any_cast<std::uint32_t>(opData.Value2));
								}
								else if (opData.ValueType == "Float")
								{
									opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), opData.ValueType, opData.FunctionType, opData.Property, std::any_cast<float>(opData.Value1), std::any_cast<float>(opData.Value2));
								}
								else if (opData.ValueType == "Bool")
								{
									opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), opData.ValueType, opData.FunctionType, opData.Property, std::any_cast<bool>(opData.Value1), std::any_cast<bool>(opData.Value2));
								}
								else if (opData.ValueType == "Enum")
								{
									opLog = fmt::format(".{}({}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), opData.ValueType, opData.FunctionType, opData.Property, std::any_cast<std::uint32_t>(opData.Value1));
								}
								else if (opData.ValueType == "FormIDInt")
								{
									opLog = fmt::format(".{}({}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), opData.ValueType, opData.FunctionType, opData.Property, std::any_cast<std::string>(opData.Value1));
								}
								else if (opData.ValueType == "FormIDFloat")
								{
									opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), opData.ValueType, opData.FunctionType, opData.Property, std::any_cast<std::string>(opData.Value1), std::any_cast<float>(opData.Value2));
								}
							}
							break;

						default:
							break;
						}

						if (opIndex == a_configData.Operations.size() - 1)
						{
							opLog += ";";
						}

						logger::info("{}    {}", indent, opLog);
					}
					break;
				}
			}

			bool ParseFilter(ConfigData& a_configData)
			{
				auto token = reader.GetToken();
				if (token == "FilterByFormID")
				{
					a_configData.Filter = FilterType::kFormID;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid FilterName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				token = reader.GetToken();
				if (token != "(")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				const auto filterFormOpt = ParseForm();
				if (!filterFormOpt.has_value())
				{
					return false;
				}

				a_configData.FilterForm = filterFormOpt.value();

				token = reader.GetToken();
				if (token != ")")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				return true;
			}

			bool ParseElement(ConfigData& a_configData)
			{
				const auto token = reader.GetToken();
				if (token == "Includes")
				{
					a_configData.Element = ElementType::kIncludes;
				}
				else if (token == "Properties")
				{
					a_configData.Element = ElementType::kProperties;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				return true;
			}

			bool ParseOperation(ConfigData& a_configData)
			{
				ConfigData::Operation newOp{};

				auto token = reader.GetToken();
				if (token == "Clear")
				{
					newOp.OpType = OperationType::kClear;
				}
				else if (token == "Add")
				{
					newOp.OpType = OperationType::kAdd;
				}
				else if (token == "Delete")
				{
					newOp.OpType = OperationType::kDelete;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				auto isValidOperation = [](ElementType elem, OperationType op) -> bool {
					switch (elem)
					{
					case ElementType::kIncludes:
					case ElementType::kProperties:
						return (op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kDelete);
					default:
						return false;
					}
				}(a_configData.Element, newOp.OpType);

				if (!isValidOperation)
				{
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element), OperationTypeToString(newOp.OpType));
					return false;
				}

				token = reader.GetToken();
				if (token != "(")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				if (a_configData.Element == ElementType::kIncludes)
				{
					if (newOp.OpType != OperationType::kClear)
					{
						ConfigData::Operation::IncludesData opData{};

						const auto formOpt = ParseForm();
						if (!formOpt.has_value())
						{
							return false;
						}
						opData.Form = formOpt.value();

						token = reader.GetToken();
						if (token != ",")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						const auto minLevelOpt = ParseNumber<std::uint8_t>();
						if (!minLevelOpt.has_value())
						{
							return false;
						}
						opData.MinLevel = minLevelOpt.value();

						token = reader.GetToken();
						if (token != ",")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						const auto optionalOpt = ParseBool();
						if (!optionalOpt.has_value())
						{
							return false;
						}
						opData.Optional = optionalOpt.value();

						token = reader.GetToken();
						if (token != ",")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						const auto dontUseAllOpt = ParseBool();
						if (!dontUseAllOpt.has_value())
						{
							return false;
						}
						opData.DontUseAll = dontUseAllOpt.value();

						newOp.OpData = opData;
					}
				}
				else if (a_configData.Element == ElementType::kProperties)
				{
					if (newOp.OpType != OperationType::kClear)
					{
						ConfigData::Operation::PropertiesData opData{};

						const auto valueTypeOpt = ParseValueType();
						if (!valueTypeOpt.has_value())
						{
							return false;
						}
						opData.ValueType = valueTypeOpt.value();

						token = reader.GetToken();
						if (token != ",")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						const auto funcTypeOpt = ParseFunctionType();
						if (!funcTypeOpt.has_value())
						{
							return false;
						}
						opData.FunctionType = funcTypeOpt.value();

						token = reader.GetToken();
						if (token != ",")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						const auto propOpt = ParseProperty();
						if (!propOpt.has_value())
						{
							return false;
						}
						opData.Property = propOpt.value();

						token = reader.GetToken();
						if (token != ",")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						auto isValidFunctionType = [](std::string_view valueType, std::string_view funcType) -> bool {
							if (valueType == "Int" || valueType == "Float")
							{
								return funcType == "SET" || funcType == "ADD" || funcType == "MULADD";
							}
							else if (valueType == "Bool")
							{
								return funcType == "SET" || funcType == "AND" || funcType == "OR";
							}
							else if (valueType == "Enum")
							{
								return funcType == "SET";
							}
							else if (valueType == "FormIDInt" || valueType == "FormIDFloat")
							{
								return funcType == "SET" || funcType == "REM" || funcType == "ADD";
							}
							return false;
						}(opData.ValueType, opData.FunctionType);

						if (!isValidFunctionType)
						{
							logger::warn("Line {}, Col {}: Invalid function type for {} '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), opData.ValueType, opData.FunctionType);
							return false;
						}

						if (opData.ValueType == "Int" || opData.ValueType == "Float")
						{
							auto parsedValueOpt = ParseNumber<float>();
							if (!parsedValueOpt.has_value())
							{
								return false;
							}

							if (opData.ValueType == "Int")
							{
								opData.Value1 = std::any(static_cast<std::uint32_t>(parsedValueOpt.value()));
							}
							else if (opData.ValueType == "Float")
							{
								opData.Value1 = std::any(parsedValueOpt.value());
							}

							token = reader.GetToken();
							if (token != ",")
							{
								logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
								return false;
							}

							parsedValueOpt = ParseNumber<float>();
							if (!parsedValueOpt.has_value())
							{
								return false;
							}

							if (opData.ValueType == "Int")
							{
								opData.Value2 = std::any(static_cast<std::uint32_t>(parsedValueOpt.value()));
							}
							else if (opData.ValueType == "Float")
							{
								opData.Value2 = std::any(parsedValueOpt.value());
							}
						}
						else if (opData.ValueType == "Bool")
						{
							auto boolOpt = ParseBool();
							if (!boolOpt.has_value())
							{
								return false;
							}

							opData.Value1 = std::any(boolOpt.value());

							token = reader.GetToken();
							if (token != ",")
							{
								logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
								return false;
							}

							boolOpt = ParseBool();
							if (!boolOpt.has_value())
							{
								return false;
							}

							opData.Value2 = std::any(boolOpt.value());
						}
						else if (opData.ValueType == "Enum")
						{
							const auto parsedValueOpt = ParseNumber<float>();
							if (!parsedValueOpt.has_value())
							{
								return false;
							}

							opData.Value1 = std::any(static_cast<std::uint32_t>(parsedValueOpt.value()));
						}
						else if (opData.ValueType == "FormIDInt" || opData.ValueType == "FormIDFloat")
						{
							const auto parsedFormOpt = ParseForm();
							if (!parsedFormOpt.has_value())
							{
								return false;
							}

							opData.Value1 = std::any(parsedFormOpt.value());

							if (opData.ValueType == "FormIDFloat")
							{
								token = reader.GetToken();
								if (token != ",")
								{
									logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
									return false;
								}

								const auto parsedNumberOpt = ParseNumber<float>();
								if (!parsedNumberOpt.has_value())
								{
									return false;
								}

								opData.Value2 = std::any(parsedNumberOpt.value());
							}
						}

						newOp.OpData = opData;
					}
				}

				token = reader.GetToken();
				if (token != ")")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				a_configData.Operations.emplace_back(newOp);

				return true;
			}

			std::optional<std::string> ParseValueType()
			{
				const auto token = reader.GetToken();
				if (token == "Int" ||
					token == "Float" ||
					token == "Bool" ||
					token == "FormIDInt" ||
					token == "Enum" ||
					token == "FormIDFloat")
				{
					return std::string(token);
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid value type '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return std::nullopt;
				}
			}

			std::optional<std::string> ParseFunctionType()
			{
				const auto token = reader.GetToken();
				if (token == "SET" ||
					token == "REM" ||
					token == "AND" ||
					token == "OR" ||
					token == "ADD" ||
					token == "MULADD")
				{
					return std::string(token);
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid function type '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return std::nullopt;
				}
			}

			std::optional<std::string> ParseProperty()
			{
				const auto token = reader.GetToken();
				if (g_propertySet.contains(token))
				{
					return std::string(token);
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid property name '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return std::nullopt;
				}
			}

			std::optional<bool> ParseBool()
			{
				const auto token = reader.GetToken();
				if (token == "true")
				{
					return true;
				}
				else if (token == "false")
				{
					return false;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid bool value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return std::nullopt;
				}
			}
		};

		void Prepare(const ConfigData& a_configData)
		{
			if (a_configData.Filter == FilterType::kFormID)
			{
				auto* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
				if (!filterForm)
				{
					logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
					return;
				}

				auto* oMod = filterForm->As<RE::BGSMod::Attachment::Mod>();
				if (!oMod)
				{
					logger::warn("'{}' is not a Object Modification.", a_configData.FilterForm);
					return;
				}

				auto& patchData = g_patchMap[oMod];

				if (a_configData.Element == ElementType::kIncludes)
				{
					if (!patchData.Includes.has_value())
					{
						patchData.Includes = PatchData::IncludesData{};
					}

					for (const auto& op : a_configData.Operations)
					{
						if (op.OpType == OperationType::kClear)
						{
							patchData.Includes->Clear = true;
						}
						else
						{
							const auto& opData = std::any_cast<const ConfigData::Operation::IncludesData&>(op.OpData.value());
							auto* includeForm = Utils::GetFormFromString(opData.Form);
							if (!includeForm)
							{
								logger::warn("Invalid FormID: '{}'.", opData.Form);
								continue;
							}

							auto* includeMod = includeForm->As<RE::BGSMod::Attachment::Mod>();
							if (!includeMod)
							{
								logger::warn("'{}' is not a Object Modification.", opData.Form);
								continue;
							}

							IncludeContainer includeContainer{};
							auto& include = reinterpret_cast<RE::BGSMod::Attachment::Instance&>(includeContainer);
							include.mod = includeMod;
							include.index = opData.MinLevel;
							include.optional = opData.Optional;
							include.childrenExclusive = opData.DontUseAll;

							if (op.OpType == OperationType::kAdd)
							{
								patchData.Includes->Add.emplace_back(includeContainer);
							}
							else
							{
								patchData.Includes->Delete.emplace_back(includeContainer);
							}
						}
					}
				}
				else if (a_configData.Element == ElementType::kProperties)
				{
					if (!patchData.Properties.has_value())
					{
						patchData.Properties = PatchData::PropertiesData{};
					}

					for (const auto& op : a_configData.Operations)
					{
						if (op.OpType == OperationType::kClear)
						{
							patchData.Properties->Clear = true;
						}
						else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete)
						{
							const auto& opData = std::any_cast<const ConfigData::Operation::PropertiesData&>(op.OpData.value());
							std::uint32_t target = 0;

							switch (oMod->targetFormType.get())
							{
							case RE::ENUM_FORM_ID::kARMO:
								{
									const auto it = g_armorPropertyMap.find(opData.Property);
									if (it == g_armorPropertyMap.end())
									{
										logger::warn("Invalid armor property: '{}'.", opData.Property);
										continue;
									}

									target = it->second;
								}
								break;

							case RE::ENUM_FORM_ID::kWEAP:
								{
									const auto it = g_weaponPropertyMap.find(opData.Property);
									if (it == g_weaponPropertyMap.end())
									{
										logger::warn("Invalid weapon property: '{}'.", opData.Property);
										continue;
									}

									target = it->second;
								}
								break;

							case RE::ENUM_FORM_ID::kNPC_:
								{
									const auto it = g_actorPropertyMap.find(opData.Property);
									if (it == g_actorPropertyMap.end())
									{
										logger::warn("Invalid actor property: '{}'.", opData.Property);
										continue;
									}

									target = it->second;
								}
								break;

							default:
								logger::warn("Cannot resolve property '{}' for unsupported Form Type: '{}'.", opData.Property, a_configData.FilterForm);
								continue;
							}

							PropertyContainer propertyContainer{};
							auto& property = reinterpret_cast<RE::BGSMod::Property::Mod&>(propertyContainer);

							property.target = target;

							if (opData.ValueType == "Int" || opData.ValueType == "Float")
							{
								if (opData.ValueType == "Int")
								{
									property.type = RE::BGSMod::Property::TYPE::kInt;
								}
								else
								{
									property.type = RE::BGSMod::Property::TYPE::kFloat;
								}

								if (opData.FunctionType == "SET")
								{
									property.op = RE::BGSMod::Property::OP::kSet;
								}
								else if (opData.FunctionType == "ADD")
								{
									property.op = RE::BGSMod::Property::OP::kAdd;
								}
								else
								{  // opData.FunctionType == "MULADD"
									property.op = RE::BGSMod::Property::OP::kMul;
								}

								if (opData.ValueType == "Int")
								{
									property.data.mm.min.i = static_cast<std::int32_t>(std::any_cast<std::uint32_t>(opData.Value1));
									property.data.mm.max.i = static_cast<std::int32_t>(std::any_cast<std::uint32_t>(opData.Value2));
								}
								else
								{
									property.data.mm.min.f = std::any_cast<float>(opData.Value1);
									property.data.mm.max.f = std::any_cast<float>(opData.Value2);
								}
							}
							else if (opData.ValueType == "Bool")
							{
								property.type = RE::BGSMod::Property::TYPE::kBool;

								if (opData.FunctionType == "SET")
								{
									property.op = RE::BGSMod::Property::OP::kSet;
								}
								else if (opData.FunctionType == "AND")
								{
									property.op = RE::BGSMod::Property::OP::kAnd;
								}
								else
								{  // opData.FunctionType == "OR"
									property.op = RE::BGSMod::Property::OP::kOr;
								}

								property.data.mm.min.i = static_cast<std::int32_t>(std::any_cast<bool>(opData.Value1));
								property.data.mm.max.i = static_cast<std::int32_t>(std::any_cast<bool>(opData.Value2));
							}
							else if (opData.ValueType == "Enum")
							{
								property.type = RE::BGSMod::Property::TYPE::kEnum;

								property.op = RE::BGSMod::Property::OP::kSet;

								property.data.mm.min.i = static_cast<std::int32_t>(std::any_cast<std::uint32_t>(opData.Value1));
							}
							else if (opData.ValueType == "FormIDInt" || opData.ValueType == "FormIDFloat")
							{
								const auto formSV = std::any_cast<std::string>(opData.Value1);

								auto* targetForm = Utils::GetFormFromString(formSV);
								if (!targetForm)
								{
									logger::warn("Invalid FormID: '{}'.", formSV);
									continue;
								}

								if (opData.ValueType == "FormIDInt")
								{
									property.type = RE::BGSMod::Property::TYPE::kForm;
								}
								else
								{
									property.type = RE::BGSMod::Property::TYPE::kPair;
								}

								if (opData.FunctionType == "SET")
								{
									property.op = RE::BGSMod::Property::OP::kSet;
								}
								else if (opData.FunctionType == "REM")
								{
									property.op = RE::BGSMod::Property::OP::kRem;
								}
								else
								{
									property.op = RE::BGSMod::Property::OP::kAdd;
								}

								if (opData.ValueType == "FormIDInt")
								{
									property.data.form = targetForm;
								}
								else
								{
									property.data.fv.formID = targetForm->formID;
									property.data.fv.value = std::any_cast<float>(opData.Value2);
								}
							}

							if (op.OpType == OperationType::kAdd)
							{
								patchData.Properties->Add.emplace_back(propertyContainer);
							}
							else
							{
								patchData.Properties->Delete.emplace_back(propertyContainer);
							}
						}
					}
				}
			}
		}

		std::vector<IncludeContainer> GetIncludes(RE::BGSMod::Attachment::Mod* a_oMod)
		{
			const auto includeBuffer = a_oMod->GetBuffer<RE::BGSMod::Attachment::Instance>(0);
			if (includeBuffer.empty())
			{
				return {};
			}

			std::vector<IncludeContainer> includes(includeBuffer.size());
			std::memcpy(includes.data(), includeBuffer.data(), includeBuffer.size_bytes());

			return includes;
		}

		std::vector<PropertyContainer> GetProperties(RE::BGSMod::Attachment::Mod* a_oMod)
		{
			const auto propertyBuffer = a_oMod->GetBuffer<RE::BGSMod::Property::Mod>(1);
			if (propertyBuffer.empty())
			{
				return {};
			}

			std::vector<PropertyContainer> properties(propertyBuffer.size());
			std::memcpy(properties.data(), propertyBuffer.data(), propertyBuffer.size_bytes());

			return properties;
		}

		void FreeBuffer(RE::BGSMod::Attachment::Mod* a_oMod, bool a_cleared)
		{
			if (!a_oMod->buffer)
			{
				return;
			}

			// Retained strings move with the copied bytes. Only Clear releases them.
			if (a_cleared)
			{
				for (auto& property : a_oMod->GetBuffer<RE::BGSMod::Property::Mod>(1))
				{
					if (property.type == RE::BGSMod::Property::TYPE::kString)
					{
						property.data.str = nullptr;
					}
				}
			}

			RE::free(a_oMod->buffer);
			a_oMod->buffer = nullptr;
			a_oMod->size = 0;
		}

		void PatchBuffer(RE::BGSMod::Attachment::Mod* a_oMod, const std::vector<IncludeContainer>& a_includes, const std::vector<PropertyContainer>& a_properties, bool a_propertiesCleared)
		{
			const auto includesSize = sizeof(RE::BGSMod::Attachment::Instance) * a_includes.size();
			const auto propertiesSize = sizeof(RE::BGSMod::Property::Mod) * a_properties.size();

			const auto bufferSize = includesSize + propertiesSize;
			if (bufferSize == 0)
			{
				FreeBuffer(a_oMod, a_propertiesCleared);
				return;
			}

			std::array<RE::BSTDataBuffer<2>::Block, 2> blocks{};
			blocks[0].id = 0;
			blocks[0].size = static_cast<std::uint32_t>(includesSize);
			blocks[1].id = 1;
			blocks[1].size = static_cast<std::uint32_t>(propertiesSize);

			auto* ptr = static_cast<std::byte*>(RE::malloc(bufferSize + sizeof(blocks)));
			if (!ptr)
			{
				logger::critical("Failed to allocate the new Object Modification buffer.");
				return;
			}

			// Includes, Properties, then the two control blocks.
			if (!a_includes.empty())
			{
				std::memcpy(ptr, a_includes.data(), includesSize);
			}
			if (!a_properties.empty())
			{
				std::memcpy(ptr + includesSize, a_properties.data(), propertiesSize);
			}
			std::memcpy(ptr + bufferSize, blocks.data(), sizeof(blocks));

			FreeBuffer(a_oMod, a_propertiesCleared);

			a_oMod->buffer = ptr;
			a_oMod->size = static_cast<std::uint32_t>(bufferSize);
		}

		bool PatchIncludes(std::vector<IncludeContainer>& a_includes, const PatchData::IncludesData& a_includesData)
		{
			bool modified = false;

			// Clear
			if (a_includesData.Clear)
			{
				a_includes.clear();
				modified = true;
			}
			else
			{
				// Delete
				for (const auto& targetIncludeContainer : a_includesData.Delete)
				{
					const auto& targetInclude = reinterpret_cast<const RE::BGSMod::Attachment::Instance&>(targetIncludeContainer);

					for (auto it = a_includes.begin(); it != a_includes.end(); ++it)
					{
						const auto& include = reinterpret_cast<const RE::BGSMod::Attachment::Instance&>(*it);
						if (include.mod != targetInclude.mod || include.index != targetInclude.index || include.optional != targetInclude.optional || include.childrenExclusive != targetInclude.childrenExclusive)
						{
							continue;
						}

						a_includes.erase(it);
						modified = true;
						break;
					}
				}
			}

			// Add
			for (const auto& targetIncludeContainer : a_includesData.Add)
			{
				a_includes.emplace_back(targetIncludeContainer);
				modified = true;
			}

			return modified;
		}

		bool PatchProperties(std::vector<PropertyContainer>& a_properties, const PatchData::PropertiesData& a_propertiesData)
		{
			bool cleared = false, modified = false;

			// Clear
			if (a_propertiesData.Clear)
			{
				a_properties.clear();
				cleared = true;
			}

			// Delete
			if (!cleared)
			{
				for (const auto& targetPropertyContainer : a_propertiesData.Delete)
				{
					const auto& targetProperty = reinterpret_cast<const RE::BGSMod::Property::Mod&>(targetPropertyContainer);

					for (auto it = a_properties.begin(); it != a_properties.end(); ++it)
					{
						const auto& property = reinterpret_cast<const RE::BGSMod::Property::Mod&>(*it);

						if (property.target != targetProperty.target || property.op != targetProperty.op || property.type != targetProperty.type)
						{
							continue;
						}

						switch (property.type)
						{
						case RE::BGSMod::Property::TYPE::kInt:
						case RE::BGSMod::Property::TYPE::kBool:
							if (property.data.mm.min.i != targetProperty.data.mm.min.i || property.data.mm.max.i != targetProperty.data.mm.max.i)
							{
								continue;
							}
							break;

						case RE::BGSMod::Property::TYPE::kFloat:
							if (property.data.mm.min.f != targetProperty.data.mm.min.f || property.data.mm.max.f != targetProperty.data.mm.max.f)
							{
								continue;
							}
							break;

						case RE::BGSMod::Property::TYPE::kForm:
							if (property.data.form != targetProperty.data.form)
							{
								continue;
							}
							break;

						case RE::BGSMod::Property::TYPE::kEnum:
							if (property.data.mm.min.i != targetProperty.data.mm.min.i)
							{
								continue;
							}
							break;

						case RE::BGSMod::Property::TYPE::kPair:
							if (property.data.fv.formID != targetProperty.data.fv.formID || property.data.fv.value != targetProperty.data.fv.value)
							{
								continue;
							}
							break;

						default:
							continue;
						}

						a_properties.erase(it);
						modified = true;
						break;
					}
				}
			}

			// Add
			for (const auto& targetPropertyContainer : a_propertiesData.Add)
			{
				a_properties.emplace_back(targetPropertyContainer);
				modified = true;
			}

			return cleared || modified;
		}
	}  // namespace

	void ReadConfigs()
	{
		g_configVec = ConfigUtils::ReadConfigs<ObjectModificationParser, Parsers::Statement<ConfigData>>(kTypeName);
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", kTypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", kTypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", kTypeName);

		for (const auto& [omod, patchData] : g_patchMap)
		{
			auto includes = GetIncludes(omod);
			auto properties = GetProperties(omod);

			bool bufferModified = false, propertiesCleared = false;

			if (patchData.Includes.has_value())
			{
				bufferModified = PatchIncludes(includes, patchData.Includes.value());
			}

			if (patchData.Properties.has_value())
			{
				const auto propertiesModified = PatchProperties(properties, patchData.Properties.value());
				bufferModified = bufferModified || propertiesModified;
				propertiesCleared = patchData.Properties->Clear;
			}

			if (bufferModified)
			{
				PatchBuffer(omod, includes, properties, propertiesCleared);
			}
		}

		logger::info("======================== Finished patching for {} ========================", kTypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}  // namespace ObjectModifications
