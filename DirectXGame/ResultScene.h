#pragma once
#include "Fade.h"
#include "KamataEngine.h"
using namespace KamataEngine;

class ResultScene {
public:
	enum class Kind { kClear, kFailed };

	explicit ResultScene(Kind kind) : kind_(kind) {}
	~ResultScene() {
		delete fade_;
		fade_ = nullptr;
	}

	void Initialize();

	void Update();

	void Draw();

	bool IsFinished() const { return finished_; }
	Kind GetKind() const { return kind_; }

private:
	enum class Phase { kFadeIn, kMain, kFadeOut };
	Phase phase_ = Phase::kFadeIn;
	const float kFadeTimeSec_ = 1.0f;

	Kind kind_;
	bool finished_ = false;
	Fade* fade_ = nullptr;

	uint32_t texClear_ = 0;
	uint32_t texFailed_ = 0;
	Sprite* clearSprite_ = nullptr;
	Sprite* failedSprite_ = nullptr;

	// ★追加：黒で“溜め”るフレーム数（見やすさ用）
	int blackHoldFrames_ = 0;
};
