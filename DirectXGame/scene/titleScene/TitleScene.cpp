#include "TitleScene.h"
#include "SoundManager.h"

#include <cmath>

using namespace KamataEngine;

namespace {
constexpr float kFadeTimeSec = 1.0f;

constexpr float kFixedDeltaSec = 1.0f / 60.0f;

// タイトル上下アニメ
constexpr float kFloatAmplitudePx = 12.0f;
constexpr float kFloatCycleSec = 2.0f;

// SPACE点滅（通常/高速）
constexpr float kBlinkCycleNormalSec = 0.8f;
constexpr float kBlinkCycleFastSec = 0.2f;
} // namespace

TitleScene::~TitleScene() {
	delete fade_;
	fade_ = nullptr;

	delete titleSprite_;
	titleSprite_ = nullptr;

	delete spaceSprite_;
	spaceSprite_ = nullptr;
}

void TitleScene::Initialize(bool returningFromGame) {
	finished_ = false;
	animTimeSec_ = 0.0f;
	blinkTimeSec_ = 0.0f;
	blinkFast_ = false;

	fade_ = new Fade();
	fade_->Initialize();

	if (returningFromGame) {
		fade_->Start(Fade::Status::FadeIn, kFadeTimeSec);
		phase_ = Phase::kFadeIn;
	} else {
		phase_ = Phase::kMain;
	}

	textureHandle_ = TextureManager::Load("scene/title.png");
	titleBasePos_ = {0.0f, 0.0f};
	titleSprite_ = Sprite::Create(textureHandle_, titleBasePos_);
	titleSprite_->SetSize(Vector2(1280.0f, 720.0f));

	titleBackTextureHandle_ = TextureManager::Load("scene/back.png");
	titleBackBasePos_ = {0.0f, 0.0f};
	titleBackSprite_ = Sprite::Create(titleBackTextureHandle_, titleBasePos_);
	titleBackSprite_->SetSize(Vector2(1280.0f, 720.0f));

	spaceTextureHandle_ = TextureManager::Load("scene/title_space.png");
	spaceBasePos_ = {0.0f, 0.0f};
	spaceSprite_ = Sprite::Create(spaceTextureHandle_, spaceBasePos_);
	spaceSprite_->SetSize(Vector2(1280.0f, 720.0f));

	SoundManager::Instance().PlayBgmTitle(0.25f);
}

void TitleScene::Update() {
	animTimeSec_ += kFixedDeltaSec;
	blinkTimeSec_ += kFixedDeltaSec;

	if (titleSprite_) {
		const float phase = (2.0f * 3.14159265f) * (animTimeSec_ / kFloatCycleSec);
		const float offsetY = std::sinf(phase) * kFloatAmplitudePx;
		titleSprite_->SetPosition({titleBasePos_.x, titleBasePos_.y + offsetY});
	}

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain: {
		auto* input = Input::GetInstance();
		const bool pressedSpace = input->TriggerKey(DIK_SPACE) || input->PushKey(DIK_SPACE);
		if (pressedSpace) {
			SoundManager::Instance().PlaySeDecision(0.85f);
			blinkFast_ = true;
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec);
			phase_ = Phase::kFadeOut;
		}
	} break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

void TitleScene::Draw() {
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());

	if (titleBackSprite_) {
		titleBackSprite_->Draw();
	}

	if (titleSprite_) {
		titleSprite_->Draw();
	}

	if (spaceSprite_) {
		const float cycle = blinkFast_ ? kBlinkCycleFastSec : kBlinkCycleNormalSec;
		const float t = std::fmod(blinkTimeSec_, cycle);
		const float alpha = (t < cycle * 0.5f) ? 1.0f : 0.0f;

		spaceSprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});
		spaceSprite_->Draw();
	}

	Sprite::PostDraw();

	if (fade_) {
		fade_->Draw();
	}
}
