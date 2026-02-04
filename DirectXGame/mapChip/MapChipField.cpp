#include "MapChipField.h"
#include <assert.h>
#include <fstream>
#include <sstream>

using namespace KamataEngine;

namespace {

bool TryParseTileCode(const std::string& word, int& outKindDigit, int& outParam) {
	// 空や非数値は無効
	if (word.empty()) {
		return false;
	}

	// 0,1,2... や 154 などを想定
	int v = 0;
	try {
		size_t idx = 0;
		v = std::stoi(word, &idx, 10);
		if (idx != word.size()) {
			return false;
		}
	} catch (...) {
		return false;
	}

	if (v < 0) {
		return false;
	}

	if (v == 0) {
		outKindDigit = 0;
		outParam = 0;
		return true;
	}

	// 1桁目（百の位ではなく “先頭の桁”）が必要なので、文字列ベースで取る
	const char c0 = word[0];
	if (c0 < '0' || c0 > '9') {
		return false;
	}

	outKindDigit = (c0 - '0');

	// 残り桁を param 扱い（無ければ0）
	if (word.size() == 1) {
		outParam = 0;
		return true;
	}

	outParam = 0;
	try {
		outParam = std::stoi(word.substr(1));
	} catch (...) {
		outParam = 0;
	}
	return true;
}

MapChipType KindDigitToType(int kindDigit) {
	switch (kindDigit) {
	case 0:
		return MapChipType::kBlank;
	case 1:
		return MapChipType::kBlock;
	case 2:
		return MapChipType::kDamage;
	case 3:
		return MapChipType::kLadder;
	case 4:
		return MapChipType::kCrumbleFloor;
	default:
		return MapChipType::kBlank;
	}
}

} // namespace

void MapChipField::ResetMapChipData() {
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVertical_);
	mapChipData_.params.clear();
	mapChipData_.params.resize(kNumBlockVertical_);

	for (uint32_t i = 0; i < kNumBlockVertical_; ++i) {
		mapChipData_.data[i].resize(kNumBlockHorizontal_);
		mapChipData_.params[i].resize(kNumBlockHorizontal_);
	}

	ladderPositions_.clear();
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
	ResetMapChipData();

	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	std::stringstream mapChipCsv;
	mapChipCsv << file.rdbuf();
	file.close();

	for (uint32_t y = 0; y < kNumBlockVertical_; ++y) {
		std::string line;
		getline(mapChipCsv, line);
		std::istringstream line_stream(line);

		for (uint32_t x = 0; x < kNumBlockHorizontal_; ++x) {
			std::string word;
			getline(line_stream, word, ',');

			int kindDigit = 0;
			int param = 0;
			if (TryParseTileCode(word, kindDigit, param)) {
				mapChipData_.data[y][x] = KindDigitToType(kindDigit);
				mapChipData_.params[y][x] = param;
			}
		}
	}

	ladderPositions_.clear();
	ladderPositions_.reserve(static_cast<size_t>(kNumBlockVertical_ * kNumBlockHorizontal_ / 8));

	for (uint32_t y = 0; y < kNumBlockVertical_; ++y) {
		for (uint32_t x = 0; x < kNumBlockHorizontal_; ++x) {
			if (mapChipData_.data[y][x] == MapChipType::kLadder) {
				ladderPositions_.push_back(GetMapChipPositionByIndex(x, y));
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (kNumBlockHorizontal_ - 1 < xIndex) {
		return MapChipType::kBlank;
	}
	if (kNumBlockVertical_ - 1 < yIndex) {
		return MapChipType::kBlank;
	}
	return mapChipData_.data[yIndex][xIndex];
}

int MapChipField::GetMapChipParamByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (kNumBlockHorizontal_ - 1 < xIndex) {
		return 0;
	}
	if (kNumBlockVertical_ - 1 < yIndex) {
		return 0;
	}
	return mapChipData_.params[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) { return Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVertical_ - 1 - yIndex), 0); }

IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position) {
	IndexSet indexSet = {};

	float adjustedX = position.x + kBlockWidth / 2.0f;
	float adjustedY = position.y + kBlockHeight / 2.0f;

	indexSet.xIndex = static_cast<uint32_t>(adjustedX / kBlockWidth);

	uint32_t reversedY = static_cast<uint32_t>(adjustedY / kBlockHeight);
	indexSet.yIndex = kNumBlockVertical_ - 1 - reversedY;

	return indexSet;
}

Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {
	Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;
	rect.left = center.x - kBlockWidth / 2.0f;
	rect.right = center.x + kBlockWidth / 2.0f;
	rect.bottom = center.y - kBlockHeight / 2.0f;
	rect.top = center.y + kBlockHeight / 2.0f;

	return rect;
}

void MapChipField::SetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex, MapChipType type) {
	if (kNumBlockHorizontal_ - 1 < xIndex) {
		return;
	}
	if (kNumBlockVertical_ - 1 < yIndex) {
		return;
	}
	mapChipData_.data[yIndex][xIndex] = type;
}

void MapChipField::SetMapChipParamByIndex(uint32_t xIndex, uint32_t yIndex, int param) {
	if (kNumBlockHorizontal_ - 1 < xIndex) {
		return;
	}
	if (kNumBlockVertical_ - 1 < yIndex) {
		return;
	}
	mapChipData_.params[yIndex][xIndex] = param;
}

bool MapChipField::IsSolid(MapChipType t) const { return (t == MapChipType::kBlock) || (t == MapChipType::kDamage) || (t == MapChipType::kCrumbleFloor); }