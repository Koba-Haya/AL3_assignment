#pragma once
#include "Fade.h"
#include "KamataEngine.h"
using namespace KamataEngine;

class ResultScene {
public:
	enum class Kind { kClear, kFailed };

	explicit ResultScene(Kind kind) : kind_(kind) {}
	~ResultScene() {
		delete clearSprite_;
		clearSprite_ = nullptr;

		delete failedSprite_;
		failedSprite_ = nullptr;

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

	Sprite* backSprite_ = nullptr;
	uint32_t backTextureHandle_ = 0;

	Sprite* spaceSprite_ = nullptr;
	uint32_t spaceTextureHandle_ = 0;

	int blackHoldFrames_ = 0;

	Vector2 basePos_ = {0.0f, 0.0f};
	Vector2 backBasePos_ = {0.0f, 0.0f};
	Vector2 spaceBasePos_ = {0.0f, 0.0f};

	float animTimeSec_ = 0.0f;
	float blinkTimeSec_ = 0.0f;
	bool blinkFast_ = false;
};
