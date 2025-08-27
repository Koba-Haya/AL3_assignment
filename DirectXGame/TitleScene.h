#pragma once
#include "Fade.h"
#include "KamataEngine.h"

using namespace KamataEngine;

class TitleScene {
public:
	~TitleScene();
	void Initialize(bool returningFromGame = false); // ★引数追加
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	enum class Phase {
		kFadeIn,  // ★ゲームから戻った直後の黒→透明
		kMain,    // 入力待ち（透明）
		kFadeOut, // ゲームへ行くための透明→黒
	};
	Phase phase_ = Phase::kFadeIn;

	bool finished_ = false;
	Fade* fade_ = nullptr;

	Sprite* titleSprite_ = nullptr;
	uint32_t textureHandle_ = 0;
};
