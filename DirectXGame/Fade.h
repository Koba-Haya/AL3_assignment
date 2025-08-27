#pragma once
#include "KamataEngine.h"
#include <algorithm>

using namespace KamataEngine;

class Fade {
public:
	// フェード状態
	enum class Status {
		None,   // フェードなし
		FadeIn, // フェードイン中（黒→透明）
		FadeOut // フェードアウト中（透明→黒）
	};

public:
	// 画面サイズはデフォルト 1280x720。必要なら指定してね
	void Initialize();

	// 任意タイミングでフェードを開始
	// durationSec: 継続時間（秒）
	void Start(Status status, float durationSec);
	void Stop();

	// 固定Δt版（教材どおり 1/60 を内部で加算）
	void Update();

	// 最前面に描画（PreDraw〜PostDraw含む）
	void Draw();

	// 便利系
	bool IsFinished() const;
	float GetAlpha() const { return alpha_; } // 現在の黒のアルファ

	Status GetStatus() const { return status_; }
	bool IsActive() const { return status_ != Status::None; }
	void UpdateInternal(float deltaSeconds);

private:
	Sprite* sprite_ = nullptr;
	uint32_t textureHandle_ = 0;

	Status status_ = Status::None;
	float duration_ = 0.0f; // フェード総時間[sec]
	float counter_ = 0.0f;  // 経過時間[sec]
	float alpha_ = 0.0f;    // 現在アルファ（0〜1）
	bool finished_ = false; // そのフェーズを完走したか

	// 画面サイズ
	uint32_t screenW_ = 1280;
	uint32_t screenH_ = 720;
};