#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

enum class MapChipType {
	kBlank,
	kBlock,
	kDamage,
	kLadder,
	kCrumbleFloor,
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
	std::vector<std::vector<int>> params;
};

struct IndexSet {
	uint32_t xIndex;
	uint32_t yIndex;
};

struct Rect {
	float left;
	float right;
	float bottom;
	float top;
};

class MapChipField {
public:
	void ResetMapChipData();
	void LoadMapChipCsv(const std::string& filePath);

	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);
	int GetMapChipParamByIndex(uint32_t xIndex, uint32_t yIndex);

	void SetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex, MapChipType type);
	void SetMapChipParamByIndex(uint32_t xIndex, uint32_t yIndex, int param);

	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	uint32_t GetNumBlockVertical() { return kNumBlockVertical_; }
	uint32_t GetNumBlockHorizontal() { return kNumBlockHorizontal_; }

	IndexSet GetMapChipIndexSetByPosition(const Vector3& position);
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

	const std::vector<Vector3>& GetLadderPositions() const { return ladderPositions_; }

	bool IsSolid(MapChipType t) const;

private:
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;
	static inline const uint32_t kNumBlockVertical_ = 20;
	static inline const uint32_t kNumBlockHorizontal_ = 100;

	MapChipData mapChipData_;
	std::vector<Vector3> ladderPositions_;
};