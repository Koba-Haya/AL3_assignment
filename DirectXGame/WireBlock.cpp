#define NOMINMAX
#include "WireBlock.h"

using namespace KamataEngine;

void WireBlock::Initialize(int maxBlocks) {
	maxBlocks_ = std::max(1, maxBlocks);

	targetCircleWT_.Initialize();
	WorldTransformUpdate(targetCircleWT_);

	lineBlocksWT_.clear();
	lineBlocksWT_.reserve(static_cast<size_t>(maxBlocks_));

	for (int i = 0; i < maxBlocks_; ++i) {
		auto wt = std::make_unique<WorldTransform>();
		wt->Initialize();
		wt->scale_ = {0.0f, 0.0f, 0.0f};
		WorldTransformUpdate(*wt);
		lineBlocksWT_.push_back(std::move(wt));
	}
}

void WireBlock::SetBlockSpacing(float s) { blockSpacing_ = std::max(0.05f, s); }

void WireBlock::Update(const Vector3& playerPos, const Vector3& cursorWorldPos) {
	Vector3 to = {cursorWorldPos.x - playerPos.x, cursorWorldPos.y - playerPos.y, cursorWorldPos.z - playerPos.z};
	float len = Length(to);

	if (len < 1e-6f) {
		activeCount_ = 0;

		targetCircleWT_.scale_ = {0.0f, 0.0f, 0.0f};
		WorldTransformUpdate(targetCircleWT_);

		for (auto& wtPtr : lineBlocksWT_) {
			wtPtr->scale_ = {0.0f, 0.0f, 0.0f};
			WorldTransformUpdate(*wtPtr);
		}
		return;
	}

	Vector3 dir = Normalize(to);

	float useDist = std::min(len, maxLength_);
	clampedTargetPos_ = {playerPos.x + dir.x * useDist, playerPos.y + dir.y * useDist, playerPos.z + dir.z * useDist};
	clampedTargetPos_.y += groundOffsetY_;

	// 目標地点の円
	targetCircleWT_.translation_ = clampedTargetPos_;
	targetCircleWT_.rotation_ = {0.0f, 0.0f, 0.0f};
	targetCircleWT_.scale_ = circleScale_;
	WorldTransformUpdate(targetCircleWT_);

	// ブロック列
	Vector3 start = {playerPos.x, playerPos.y + groundOffsetY_, playerPos.z};
	Vector3 end = clampedTargetPos_;
	Vector3 seg = {end.x - start.x, end.y - start.y, end.z - start.z};
	float segLen = Length(seg);

	if (segLen < 1e-4f) {
		activeCount_ = 0;
		for (auto& wtPtr : lineBlocksWT_) {
			wtPtr->scale_ = {0.0f, 0.0f, 0.0f};
			WorldTransformUpdate(*wtPtr);
		}
		return;
	}

	Vector3 segDir = Normalize(seg);

	int count = static_cast<int>(segLen / blockSpacing_) + 1;
	count = std::clamp(count, 1, maxBlocks_);
	activeCount_ = count;

	for (int i = 0; i < maxBlocks_; ++i) {
		auto& wtPtr = lineBlocksWT_[static_cast<size_t>(i)];
		WorldTransform& wt = *wtPtr;

		if (i >= count) {
			wt.scale_ = {0.0f, 0.0f, 0.0f};
			WorldTransformUpdate(wt);
			continue;
		}

		float t = 0.0f;
		if (count > 1) {
			t = static_cast<float>(i) / static_cast<float>(count - 1);
		}

		Vector3 p = {start.x + segDir.x * (segLen * t), start.y + segDir.y * (segLen * t), start.z + segDir.z * (segLen * t)};

		wt.translation_ = p;
		wt.rotation_ = {0.0f, 0.0f, 0.0f};
		wt.scale_ = blockScale_;
		WorldTransformUpdate(wt);
	}
}
