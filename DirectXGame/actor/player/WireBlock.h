#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"

#include <algorithm>
#include <memory>
#include <vector>

using namespace KamataEngine;

class WireBlock {
public:
	void Initialize(int maxBlocks);

	void SetMaxLength(float maxLen) { maxLength_ = maxLen; }
	void SetBlockSpacing(float s);
	void SetGroundOffsetY(float y) { groundOffsetY_ = y; }

	void Update(const Vector3& playerPos, const Vector3& cursorWorldPos);

	const WorldTransform& GetTargetCircleWT() const { return targetCircleWT_; }
	const std::vector<std::unique_ptr<WorldTransform>>& GetLineBlockWTs() const { return lineBlocksWT_; }

	Vector3 GetClampedTargetPos() const { return clampedTargetPos_; }
	int GetActiveBlockCount() const { return activeCount_; }

	void SetCircleScale(const Vector3& s) { circleScale_ = s; }
	void SetBlockScale(const Vector3& s) { blockScale_ = s; }

private:
	float maxLength_ = 12.0f;
	float blockSpacing_ = 0.6f;
	int maxBlocks_ = 64;
	float groundOffsetY_ = 0.02f;

	Vector3 clampedTargetPos_{0, 0, 0};

	WorldTransform targetCircleWT_{};
	std::vector<std::unique_ptr<WorldTransform>> lineBlocksWT_;
	int activeCount_ = 0;

	Vector3 circleScale_{1.0f, 1.0f, 1.0f};
	Vector3 blockScale_{0.2f, 0.2f, 0.2f};
};
