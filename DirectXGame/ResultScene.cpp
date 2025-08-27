#include "ResultScene.h"

void ResultScene::Initialize() {
	finished_ = false;
	fade_ = new Fade();
	fade_->Initialize();
	// 入場は黒→透明
	fade_->Start(Fade::Status::FadeIn, kFadeTimeSec_);
	phase_ = Phase::kFadeIn;

	// ★画像ロード
	texClear_ = TextureManager::Load("scene/clear.png");
	texFailed_ = TextureManager::Load("scene/failed.png");

	// ★スプライト生成（画面中央に配置）
	// 画面サイズは 1280x720 を想定。違う場合は数値を変えてね。
	const Vector2 screen = {1280.0f, 720.0f};

	// 任意のサイズ（画像そのままでもOK）
	const Vector2 size = {1280.0f, 720.0f};
	const Vector2 pos = {(screen.x - size.x) * 0.5f, (screen.y - size.y) * 0.5f};

	clearSprite_ = Sprite::Create(texClear_, pos);
	clearSprite_->SetSize(size);

	failedSprite_ = Sprite::Create(texFailed_, pos);
	failedSprite_->SetSize(size);
}

void ResultScene::Update() {
	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;
	case Phase::kMain: {
		auto* in = Input::GetInstance();
		if (in->TriggerKey(DIK_SPACE) || in->PushKey(DIK_SPACE)) {
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec_);
			blackHoldFrames_ = 0;                 
			phase_ = Phase::kFadeOut;
		}
	} break;
	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			if (blackHoldFrames_ < 2) { // 1〜2フレームで十分に見える
				++blackHoldFrames_;
			} else {
				// 黒で“溜め”たらシーン終了（黒のまま次へ）
				finished_ = true;
			}
		}
		break;
	}
}

void ResultScene::Draw() {
	// ① 結果スプライト（CLEAR or FAILED）を描く
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
	if (kind_ == Kind::kClear) {
		if (clearSprite_) {
			clearSprite_->Draw();
		}
	} else {
		if (failedSprite_) {
			failedSprite_->Draw();
		}
	}
	Sprite::PostDraw();

	// ② その上にフェード（最前面）
	if (fade_) {
		fade_->Draw();
	}
}
