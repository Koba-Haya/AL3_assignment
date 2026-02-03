#define NOMINMAX 
#include "Fade.h"
using namespace KamataEngine;

void Fade::Initialize() {

	// 1x1白テクスチャ推奨（色で黒にする）
	textureHandle_ = TextureManager::Load("black1x1.png");

	// ★重要：sprite_->Create(...) ではなく Sprite::Create(...) で生成
	sprite_ = Sprite::Create(textureHandle_, {0.0f, 0.0f});
	sprite_->SetSize(Vector2(static_cast<float>(screenW_), static_cast<float>(screenH_)));
	// 初期は非表示（透明な黒）
	alpha_ = 0.0f;
	sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f));

	status_ = Status::None;
	duration_ = 0.0f;
	counter_ = 0.0f;
	finished_ = false;
}

void Fade::Start(Status status, float durationSec) {
	status_ = status;
	duration_ = std::max(0.0001f, durationSec); // 0除算よけ
	counter_ = 0.0f;

	// 開始直後の表示を整える
	switch (status_) {
	case Status::FadeIn: // 黒→透明：開始時は黒
		sprite_->SetColor(Vector4(0, 0, 0, 1));
		break;
	case Status::FadeOut: // 透明→黒：開始時は透明
		sprite_->SetColor(Vector4(0, 0, 0, 0));
		break;
	default:
		break;
	}
}

void Fade::Update() {
	// 教材どおり固定 1/60 秒加算
	UpdateInternal(1.0f / 60.0f);
}

void Fade::Stop() {
    status_ = Status::None;
}

bool Fade::IsFinished() const {
	if (status_ == Status::None)
		return true;
	// FadeIn / FadeOut 共通で「経過時間が満了したら完了」
	return counter_ >= duration_;
}

void Fade::UpdateInternal(float deltaSeconds) {
	counter_ += deltaSeconds;
	float t = std::clamp(counter_ / duration_, 0.0f, 1.0f);

	switch (status_) {
	case Status::None:
		break;
	case Status::FadeOut:
		// 透明(0) → 黒(1) に増加
		alpha_ = t; // スクショの式：clamp(counter_/duration, 0, 1)
		break;
	case Status::FadeIn:
		// 黒(1) → 透明(0) に減少
		alpha_ = 1.0f - t;
		break;
	default:
		break;
	}

	sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, alpha_));

	// 打ち止め
	if (counter_ >= duration_) {
		finished_ = true;
		counter_ = duration_;
		// フェーズ完了後も黒板を残したい時は None にしない
		// 自動で終了したいなら↓を有効化
		// status_ = Status::None;
	}
}

void Fade::Draw() {
	if (status_==Status::None) {
		return;
	}
	// フェードは最前面に出したいので、単独で PreDraw→Draw→PostDraw
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
	sprite_->Draw();
	Sprite::PostDraw();
}