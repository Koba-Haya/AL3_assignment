#pragma once
#include "Fade.h"
#include "KamataEngine.h"

using namespace KamataEngine;

class TitleScene {
public:
	~TitleScene();
	void Initialize(bool returningFromGame = false);
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	enum class Phase {
		kFadeIn,  // ゲームから戻った直後の黒→透明
		kMain,    // 入力待ち（透明）
		kFadeOut, // ゲームへ行くための透明→黒
	};
	Phase phase_ = Phase::kFadeIn;

	bool finished_ = false;
	Fade* fade_ = nullptr;

	Sprite* titleSprite_ = nullptr;
	uint32_t textureHandle_ = 0;

	Sprite* titleBackSprite_ = nullptr;
	uint32_t titleBackTextureHandle_ = 0;

	Sprite* spaceSprite_ = nullptr;
	uint32_t spaceTextureHandle_ = 0;

	Vector2 titleBasePos_ = {0.0f, 0.0f};
	Vector2 titleBackBasePos_ = {0.0f, 0.0f};
	Vector2 spaceBasePos_ = {0.0f, 0.0f};
	float animTimeSec_ = 0.0f;
	float blinkTimeSec_ = 0.0f;
	bool blinkFast_ = false;
};
