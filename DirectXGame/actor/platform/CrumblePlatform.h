#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"
#include "MapChipField.h"

using namespace KamataEngine;

class MapChipField;

class CrumblePlatform {
public:
	void Initialize(Model* model, Camera* camera, const Vector3& pos, int reviveSec, int fallSec, MapChipField* mapChipField);

	void Update();
	void Draw() const;

	void OnStepped(); // 踏まれたらカウント開始

	bool IsActive() const { return active_; }
	AABB GetAABB() const { return aabb_; }

private:
	enum class State { Stable, Counting, Falling, WaitingRevive };

	void RebuildAABB_();

	Model* model_ = nullptr;
	Camera* camera_ = nullptr;

	WorldTransform worldTransform_{};
	AABB aabb_{};

	MapChipField* mapChipField_ = nullptr;
	IndexSet tileIndex_{0, 0};
	Vector3 homePos_{0.0f, 0.0f, 0.0f};

	bool active_ = true;
	State state_ = State::Stable;

	float fallTimeSec_ = 1.0f;
	float reviveTimeSec_ = 3.0f;

	float fallLeft_ = 0.0f;
	float reviveLeft_ = 0.0f;

	float fallSpeed_ = 0.2f;      // 1フレームで落ちる量
	float vanishY_ = -30.0f;       // これより下に行ったら画面外扱い

	static inline const float kFixedDt = 1.0f / 60.0f;
};