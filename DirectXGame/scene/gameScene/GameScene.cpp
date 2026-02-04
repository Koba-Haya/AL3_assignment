#define NOMINMAX

#include "GameScene.h"
#include "SoundManager.h"
#include <Windows.h>
#include <cmath>

using namespace KamataEngine;

namespace {
constexpr float kFadeTimeSec = 1.0f;
constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;
} // namespace

GameScene::~GameScene() {
	// 3Dモデルデータの開放
	delete playerModel_;
	// 3Dモデルデータの開放
	delete enemyModel_;
	// デスパーティクルモデルの開放
	delete particleModel_;
	// ブロックモデルデータの開放
	delete modelBlock_;
	// 天球モデルデータの開放
	delete modelSkydome_;
	// デバッグカメラの開放
	delete debugCamera_;
	// 自キャラの開放
	delete player_;
	// 天球の開放
	delete skydome_;
	// デスパーティクルの開放
	delete deathParticles_;
	// フェードの開放
	delete fade_;
	// 梯子モデルの開放
	delete modelLadder_;
	// ゴールの開放
	delete goal_;
	goal_ = nullptr;
	// Ammo UIの開放
	for (int i = 0; i < kDigitCount; ++i) {
		delete ammoCurDigits_[i];
		ammoCurDigits_[i] = nullptr;
		delete ammoMaxDigits_[i];
		ammoMaxDigits_[i] = nullptr;
	}
	delete ammoSlashSprite_;
	ammoSlashSprite_ = nullptr;
	// 敵キャラの開放
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	// マップチップフィールドの開放
	delete mapChipField_;

	// ブロックモデルの開放
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();

	// ダメージブロックモデルの開放
	delete modelDamageBlock_;
	modelDamageBlock_ = nullptr;

	// ダメージブロックのワールド変換データの開放
	for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
		for (WorldTransform* wt : line) {
			delete wt;
		}
	}
	worldTransformDamageBlocks_.clear();

	// 梯子Transformの解放
	for (WorldTransform* wt : ladderWorldTransforms_) {
		delete wt;
	}
	ladderWorldTransforms_.clear();

	for (CrumblePlatform* p : crumblePlatforms_) {
		delete p;
	}
	crumblePlatforms_.clear();

	// 弾モデルの開放
	delete bulletModel_;
	bulletModel_ = nullptr;
}

void GameScene::Initialize() {

	// 3Dモデルデータの生成
	playerModel_ = Model::CreateFromOBJ("player", true);
	// 3Dモデルデータの生成
	enemyModel_ = Model::CreateFromOBJ("enemy", true);
	// デスパーティクルモデルの生成
	particleModel_ = Model::CreateFromOBJ("particle", true);
	// ブロックモデルデータの生成
	modelBlock_ = Model::CreateFromOBJ("cube", true);
	// 天球モデルデータの生成
	modelSkydome_ = Model::CreateFromOBJ("sky_sphere", true);
	// ダメージブロックモデル
	modelDamageBlock_ = Model::CreateFromOBJ("damageBlock", true);
	// 梯子モデルの生成
	modelLadder_ = Model::CreateFromOBJ("ladder", true);

	// カメラのfarZを適度に大きい値に
	camera_.farZ = 1000.0f;
	// カメラの初期化
	camera_.Initialize();
	camera_.translation_.z = -30.0f;

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	// マップチップフィールドの生成
	mapChipField_ = new MapChipField;
	// CSVファイルからマップデータを読み込み
	mapChipField_->LoadMapChipCsv("Resources/block.csv");

	// 自キャラの生成
	player_ = new Player;
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);
	// 自キャラの初期化
	player_->Initialize(playerModel_, &camera_, playerPosition);
	// マップチップデータのセット
	player_->SetMapChipField(mapChipField_);

	// 敵を複数生成
	const int enemyCount = 3;
	for (int32_t i = 0; i < enemyCount; ++i) {
		Enemy* newEnemy = new Enemy();

		// 例：プレイヤーと同じ「地面の1つ上の段」に置く
		// xIndex は適当にずらす（例: 6, 10, 14）
		Vector3 enemyPosition;
		if (i == 0) {
			enemyPosition = mapChipField_->GetMapChipPositionByIndex(6, 14);
		} else if (i == 1) {
			enemyPosition = mapChipField_->GetMapChipPositionByIndex(14, 18);
		} else {
			enemyPosition = mapChipField_->GetMapChipPositionByIndex(18, 18);
		}

		newEnemy->Initialize(enemyModel_, &camera_, enemyPosition);

		newEnemy->SetMapChipField(mapChipField_);
		enemies_.push_back(newEnemy);
	}

	// 天球の生成
	skydome_ = new Skydome;
	// 天球の初期化
	skydome_->Initialize();

	GenerateBlocks();

	// ゲームプレイフェーズから開始
	phase_ = Phase::kPlay;

	Model* goalModel = Model::CreateFromOBJ("goal", true);
	Vector3 goalPos = mapChipField_->GetMapChipPositionByIndex(30, 18);

	goal_ = new Goal();
	goal_->Initialize(goalModel, goalPos);

	// フェード
	fade_ = new Fade();
	fade_->Initialize();

	// プレイヤーの弾モデルの生成
	bulletModel_ = Model::CreateFromOBJ("bullet", true);

	// ゲーム開始時は黒→透明のフェードイン。完了までは動かさない
	fade_->Start(Fade::Status::FadeIn, kFadeTimeSec);
	phase_ = Phase::kFadeIn;

	// カメラコントローラーの初期化
	cameraController_ = new CameraController; // 生成
	cameraController_->Initialize();          // 初期化
	cameraController_->SetTarget(player_);    // 追従対象をセット
	cameraController_->Reset();               // リセット（瞬間合わせ）

	textureHandle_ = TextureManager::Load("scene/move.png");
	// ★重要：sprite_->Create(...) ではなく Sprite::Create(...) で生成
	moveSprite_ = Sprite::Create(textureHandle_, {0.0f, 0.0f});
	moveSprite_->SetSize(Vector2(1280.0f, 720.0f));

	// ワイヤービジュアライザー用モデルの生成
	wireBlockModel_ = Model::CreateFromOBJ("block", true);

	// ワイヤー目標円モデルの生成
	wireCircleModel_ = Model::CreateFromOBJ("circle", true);

	wireViz_.Initialize(64);
	wireViz_.SetMaxLength(12.0f);
	wireViz_.SetBlockSpacing(0.6f);
	wireViz_.SetGroundOffsetY(0.02f);

	wireViz_.SetCircleScale({1.0f, 1.0f, 1.0f});
	// ブロックのサイズ
	wireViz_.SetBlockScale({0.2f, 0.2f, 0.2f});

	playedDeathSe_ = false;
	playedGoalSe_ = false;

	SoundManager::Instance().PlayBgmGame(0.25f);

	// ---- Ammo UI ----
	for (int i = 0; i < 10; ++i) {
		const std::string path = "ui/" + std::to_string(i) + ".png";
		digitTex_[i] = TextureManager::Load(path);
	}
	slashTex_ = TextureManager::Load("ui/slash.png");

	const Vector2 base = {1060.0f, 20.0f};
	const Vector2 digitSize = {48.0f, 64.0f};

	for (int i = 0; i < kDigitCount; ++i) {
		ammoCurDigits_[i] = Sprite::Create(digitTex_[i], base);
		ammoCurDigits_[i]->SetSize(digitSize);

		ammoMaxDigits_[i] = Sprite::Create(digitTex_[i], {base.x + 104.0f, base.y});
		ammoMaxDigits_[i]->SetSize(digitSize);
	}

	ammoSlashSprite_ = Sprite::Create(slashTex_, {base.x + 52.0f, base.y});
	ammoSlashSprite_->SetSize(digitSize);
}

void GameScene::Update() {
	ChangePhase();

	switch (phase_) {
	case Phase::kFadeIn:

		if (isDebugCameraActive_) {
			debugCamera_->Update();
			// DebugCamera から Camera を取得し、camera_ にコピー
			camera_.matView = debugCamera_->GetCamera().matView;
			camera_.matProjection = debugCamera_->GetCamera().matProjection;

			camera_.TransferMatrix();
		} else {
			// カメラコントローラーの更新
			cameraController_->Update();

			// カメラを controller から取得して camera_ に反映
			const Camera& controlledCam = cameraController_->GetCamera();
			camera_.matView = controlledCam.matView;
			camera_.matProjection = controlledCam.matProjection;

			// ここで行列転送も必要（たとえば TransferMatrix などが必要なら）
			camera_.TransferMatrix();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				// アフィン変換行列の作成
				Matrix4x4 blockAffineMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
				// ワールド行列に代入
				worldTransformBlock->matWorld_ = blockAffineMatrix;
				// 定数バッファの転送
				worldTransformBlock->TransferMatrix();
			}
		}
		// ダメージブロックの更新（matWorld_/CB転送）
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				Matrix4x4 affine = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->matWorld_ = affine;
				wt->TransferMatrix();
			}
		}

		if (player_) {
			player_->UpdateFreeze();
		}
		for (auto* e : enemies_) {
			e->UpdateFreeze();
		}

		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop(); // 完了したら止めて描画コスト削減
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay: {

		if (goal_) {
			goal_->Update();
		}

		// ★Jキーでワイヤー発射/解除（トグル）
		if (player_ && Input::GetInstance()->TriggerKey(DIK_J)) {
			player_->ToggleWire();
			if (player_->IsWiring()) {
				SoundManager::Instance().PlaySeWireMove(0.75f);
			}
		}

		// 可視化更新：ワイヤー中だけ（アンカー or 発射中の先端）
		if (player_ && player_->IsWiring()) {
			const Vector3 playerPos = player_->GetWorldTransform().translation_;
			const Vector3 target = player_->GetWireVisualTarget();
			wireViz_.Update(playerPos, target);
		}

		// 床更新（先に動かす）
		for (CrumblePlatform* p : crumblePlatforms_) {
			p->Update();
		}

		// 自キャラ更新など既存処理
		player_->Update();

		// ---- 連射クールダウン更新（追加）----
		const float dt = 1.0f / 60.0f;
		shootCooldown_ = std::max(0.0f, shootCooldown_ - dt);

		// ---- 攻撃入力（ワイヤー/はしご中は Player 側で弾く）----
		if (player_) {
			// 近距離（突進）
			if (Input::GetInstance()->TriggerKey(DIK_K)) {
				player_->BeginDashAttack();
			}

			// 遠距離（射撃）: 長押し連射
			if (Input::GetInstance()->PushKey(DIK_L) && shootCooldown_ <= 0.0f) {
				if (player_->TryConsumeAmmoAndStartReloadIfNeeded()) {
					const Vector3 pos = player_->GetMuzzleWorldPos();

					const float bulletSpeed = 0.45f;

					Vector3 targetPos{};
					Vector3 dir{};

					if (TryGetShootTargetPos_(targetPos)) {
						Vector3 to = {targetPos.x - pos.x, targetPos.y - pos.y, targetPos.z - pos.z};
						if (Length(to) < 1e-6f) {
							dir = player_->GetFacingDir();
						} else {
							dir = Normalize(to);
						}
					} else {
						// 敵がいないときは正面
						dir = player_->GetFacingDir();
					}

					Vector3 vel = {dir.x * bulletSpeed, dir.y * bulletSpeed, dir.z * bulletSpeed};

					auto b = std::make_unique<PlayerBullet>();
					b->Initialize(bulletModel_, &camera_, pos, vel);
					bullets_.push_back(std::move(b));

					shootCooldown_ = kShootIntervalSec;
				} else {
					shootCooldown_ = 0.06f;
				}
			}
		}

		// 弾更新
		UpdateBullets_();

		auto isStandingOn = [](const AABB& actor, const AABB& floorAabb) -> bool {
			const float epsY = 0.03f;

			const bool overlapX = (actor.max.x > floorAabb.min.x) && (actor.min.x < floorAabb.max.x);
			const bool overlapZ = (actor.max.z > floorAabb.min.z) && (actor.min.z < floorAabb.max.z);

			const float actorBottom = actor.min.y;
			const float floorTop = floorAabb.max.y;

			const bool nearTop = (actorBottom >= floorTop - 0.08f) && (actorBottom <= floorTop + epsY);
			return overlapX && overlapZ && nearTop;
		};

		// ---- 崩れる床：踏まれてたら OnStepped
		for (CrumblePlatform* p : crumblePlatforms_) {
			if (!p || !p->IsActive()) {
				continue;
			}

			const AABB fa = p->GetAABB();

			if (player_ && isStandingOn(player_->GetAABB(), fa)) {
				p->OnStepped();
				continue;
			}

			for (Enemy* e : enemies_) {
				if (!e)
					continue;
				if (isStandingOn(e->GetAABB(), fa)) {
					p->OnStepped();
					break;
				}
			}
		}

		skydome_->Update();
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

#ifdef _DEBUG
		if (Input::GetInstance()->TriggerKey(DIK_1)) { // 例：キー1で切り替え
			isDebugCameraActive_ = !isDebugCameraActive_;
		}
#endif

		if (isDebugCameraActive_) {
			debugCamera_->Update();
			// DebugCamera から Camera を取得し、camera_ にコピー
			camera_.matView = debugCamera_->GetCamera().matView;
			camera_.matProjection = debugCamera_->GetCamera().matProjection;

			camera_.TransferMatrix();
		} else {
			// カメラコントローラーの更新
			cameraController_->Update();

			// カメラを controller から取得して camera_ に反映
			const Camera& controlledCam = cameraController_->GetCamera();
			camera_.matView = controlledCam.matView;
			camera_.matProjection = controlledCam.matProjection;

			// ここで行列転送も必要（たとえば TransferMatrix などが必要なら）
			camera_.TransferMatrix();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				Matrix4x4 m = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->matWorld_ = m;
				wt->TransferMatrix();
			}
		}
		// ダメージブロックの更新（matWorld_/CB転送）
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				Matrix4x4 affine = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->matWorld_ = affine;
				wt->TransferMatrix();
			}
		}

		// すべての当たり判定を行う
		CheckAllCollisions();

		// 死亡検知→フェードアウト開始（タイトルへ戻る準備）
		if (player_->IsDead()) {

			if (!playedDeathSe_) {
				playedDeathSe_ = true;
				SoundManager::Instance().PlaySePlayerDeath(0.95f);
			}

			const Vector3& pos = player_->GetWorldPosition();
			deathParticles_ = new DeathParticles;
			deathParticles_->Initialize(particleModel_, &camera_, pos);
			phase_ = Phase::kDeath;
			return;
		}

		// ゴール到達判定（通り抜けOKのトリガー）
		if (goal_ && goal_->IsActive() && IntersectAABB(player_->GetAABB(), goal_->GetAABB())) {
			if (!playedGoalSe_) {
				playedGoalSe_ = true;
				SoundManager::Instance().PlaySeGoal(0.95f);
			}

			goal_->SetActive(false);
			result_ = Result::kClear;
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec);
			phase_ = Phase::kFadeOut;
			break;
		}

		break;
	}

	case Phase::kDeath:

		// パーティクルの更新
		if (deathParticles_) {
			deathParticles_->Update();
		}

		if (isDebugCameraActive_) {
			debugCamera_->Update();
			// DebugCamera から Camera を取得し、camera_ にコピー
			camera_.matView = debugCamera_->GetCamera().matView;
			camera_.matProjection = debugCamera_->GetCamera().matProjection;

			camera_.TransferMatrix();
		} else {
			// カメラコントローラーの更新
			cameraController_->Update();

			// カメラを controller から取得して camera_ に反映
			const Camera& controlledCam = cameraController_->GetCamera();
			camera_.matView = controlledCam.matView;
			camera_.matProjection = controlledCam.matProjection;

			// ここで行列転送も必要（たとえば TransferMatrix などが必要なら）
			camera_.TransferMatrix();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				// アフィン変換行列の作成
				Matrix4x4 blockAffineMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
				// ワールド行列に代入
				worldTransformBlock->matWorld_ = blockAffineMatrix;
				// 定数バッファの転送
				worldTransformBlock->TransferMatrix();
			}
		}
		// ダメージブロックの更新（matWorld_/CB転送）
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				Matrix4x4 affine = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->matWorld_ = affine;
				wt->TransferMatrix();
			}
		}

		if (deathParticles_ && deathParticles_->IsFinished()) {
			result_ = Result::kFailed;

			// 画面フェード（透明→黒）
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec);

			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:

		// ★フェードアウト中は基本停止。必要なら背景だけUpdateしてもOK
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true; // → タイトルへ
		}
		break;

	default:
		break;
	}
}

void GameScene::Draw() {
	switch (phase_) {

	case Phase::kFadeIn:
		// プレイヤーの描画
		player_->Draw();
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}
		// 崩れる床の描画
		for (CrumblePlatform* p : crumblePlatforms_) {
			if (p)
				p->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ダメージブロックの描画
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				modelDamageBlock_->Draw(*wt, camera_);
			}
		}

		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		// Ammo UI draw
		if (player_ && ammoSlashSprite_) {
			const int cur = std::clamp(player_->GetAmmo(), 0, 9);
			const int mx = std::clamp(player_->GetAmmoMax(), 0, 9);

			const float alpha = player_->IsReloading() ? 0.45f : 1.0f;

			// 色反映（毎フレームでOK：UIだけ）
			for (int i = 0; i < kDigitCount; ++i) {
				if (ammoCurDigits_[i]) {
					ammoCurDigits_[i]->SetColor({1.0f, 1.0f, 1.0f, alpha});
				}
				if (ammoMaxDigits_[i]) {
					ammoMaxDigits_[i]->SetColor({1.0f, 1.0f, 1.0f, alpha});
				}
			}
			ammoSlashSprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});

			// 描画（必要な数字だけ）
			if (ammoCurDigits_[cur]) {
				ammoCurDigits_[cur]->Draw();
			}
			ammoSlashSprite_->Draw();
			if (ammoMaxDigits_[mx]) {
				ammoMaxDigits_[mx]->Draw();
			}
		}

		Sprite::PostDraw();
		// フェードは最後に
		fade_->Draw();
		break;

	case Phase::kPlay: {

		// 自キャラの描画
		player_->Draw();
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}

		// ---- 弾の描画 ----
		for (auto& b : bullets_) {
			if (b) {
				b->Draw();
			}
		}

		const bool canDrawWireViz = (player_ && player_->IsWiring());
		if (canDrawWireViz) {
			// ブロック列
			if (wireBlockModel_) {
				const auto& wts = wireViz_.GetLineBlockWTs();
				const int count = wireViz_.GetActiveBlockCount();
				for (int i = 0; i < count; ++i) {
					auto& wtPtr = wts[static_cast<size_t>(i)];
					if (wtPtr) {
						wireBlockModel_->Draw(*wtPtr, camera_);
					}
				}
			}

			// 最終地点（円）
			if (wireCircleModel_) {
				wireCircleModel_->Draw(wireViz_.GetTargetCircleWT(), camera_);
			}
		}

		// 崩れる床の描画
		for (CrumblePlatform* p : crumblePlatforms_) {
			if (p)
				p->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ダメージブロックの描画
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				modelDamageBlock_->Draw(*wt, camera_);
			}
		}

		// 梯子モデルの描画（毎フレームInitializeしない）
		if (modelLadder_) {
			for (WorldTransform* wt : ladderWorldTransforms_) {
				if (!wt) {
					continue;
				}
				wt->matWorld_ = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->TransferMatrix();
				modelLadder_->Draw(*wt, camera_);
			}
		}

		// ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		// Ammo UI draw
		if (player_ && ammoSlashSprite_) {
			const int cur = std::clamp(player_->GetAmmo(), 0, 9);
			const int mx = std::clamp(player_->GetAmmoMax(), 0, 9);

			const float alpha = player_->IsReloading() ? 0.45f : 1.0f;

			// 色反映（毎フレームでOK：UIだけ）
			for (int i = 0; i < kDigitCount; ++i) {
				if (ammoCurDigits_[i]) {
					ammoCurDigits_[i]->SetColor({1.0f, 1.0f, 1.0f, alpha});
				}
				if (ammoMaxDigits_[i]) {
					ammoMaxDigits_[i]->SetColor({1.0f, 1.0f, 1.0f, alpha});
				}
			}
			ammoSlashSprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});

			// 描画（必要な数字だけ）
			if (ammoCurDigits_[cur]) {
				ammoCurDigits_[cur]->Draw();
			}
			ammoSlashSprite_->Draw();
			if (ammoMaxDigits_[mx]) {
				ammoMaxDigits_[mx]->Draw();
			}
		}

		Sprite::PostDraw();
		break;
	}
	case Phase::kDeath: {

		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}
		if (deathParticles_) {
			deathParticles_->Draw();
		}

		// 崩れる床の描画
		for (CrumblePlatform* p : crumblePlatforms_) {
			if (p)
				p->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ダメージブロックの描画
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
			 modelDamageBlock_->Draw(*wt, camera_);
			}
		}

		// ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		break;
	}
	case Phase::kFadeOut: {
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}

		// 崩れる床の描画
		for (CrumblePlatform* p : crumblePlatforms_) {
			if (p)
				p->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ダメージブロックの描画
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				modelDamageBlock_->Draw(*wt, camera_);
			}
		}

		// ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		// フェードは最後に
		fade_->Draw();
		break;
	}
	default:
		break;
	}
}

void GameScene::GenerateBlocks() {
	const uint32_t kNumBlockVirtical = mapChipField_->GetNumBlockVertical();
	const uint32_t kNumBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	worldTransformBlocks_.clear();
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	worldTransformDamageBlocks_.clear();
	worldTransformDamageBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		worldTransformDamageBlocks_[i].resize(kNumBlockHorizontal);
	}

	// ★追加：梯子WTをここで作り直す
	for (WorldTransform* wt : ladderWorldTransforms_) {
		delete wt;
	}
	ladderWorldTransforms_.clear();

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {

			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);

			if (type == MapChipType::kBlock) {
				WorldTransform* wt = new WorldTransform();
				wt->Initialize();
				wt->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransformBlocks_[i][j] = wt;

			} else if (type == MapChipType::kDamage) {
				WorldTransform* wt = new WorldTransform();
				wt->Initialize();
				wt->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransformDamageBlocks_[i][j] = wt;

			} else if (type == MapChipType::kLadder) {
				WorldTransform* wt = new WorldTransform();
				wt->Initialize();
				wt->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				ladderWorldTransforms_.push_back(wt);
			} else if (type == MapChipType::kCrumbleFloor) {
				const int param = mapChipField_->GetMapChipParamByIndex(j, i);
				const int reviveSec = std::clamp(param / 10, 1, 9);
				const int fallSec = std::clamp(param % 10, 1, 9);

				CrumblePlatform* p = new CrumblePlatform();
				p->Initialize(modelBlock_, &camera_, mapChipField_->GetMapChipPositionByIndex(j, i), reviveSec, fallSec, mapChipField_);
				crumblePlatforms_.push_back(p);
			}
		}
	}
}

// タイルRect→AABBにして判定するユーティリティ
static AABB MakeTileAabbFromRect(const Rect& r) {
	AABB a;
	a.min = {r.left, r.bottom, -0.5f};
	a.max = {r.right, r.top, +0.5f};
	return a;
}

void GameScene::CheckAllCollisions() {

	// プレイヤーの判定用データ
	AABB playerAabb = player_->GetAABB();
	Vector3 playerPos = player_->GetWorldPosition();

	// ---- プレイヤー → 敵キャラ ----
	// 突進攻撃中は接触死しない（攻撃判定を優先）
	if (player_ && !player_->IsDashAttacking()) {
		AABB aabb1 = player_->GetAABB();
		for (Enemy* enemy : enemies_) {
			if (!enemy || enemy->IsDead()) {
				continue;
			}

			AABB aabb2 = enemy->GetAABB();
			if (IsCollision(aabb1, aabb2)) {
				player_->OnCollision(enemy);
				enemy->OnCollision(player_);
			}
		}
	}

	// ---- 近距離（突進） → 敵死亡 ----
	if (player_ && player_->IsDashAttacking()) {
		const AABB atk = player_->GetDashAttackAABB();
		for (Enemy* enemy : enemies_) {
			if (!enemy || enemy->IsDead()) {
				continue;
			}
			if (IsCollision(atk, enemy->GetAABB())) {
				enemy->TakeDamage(9999);
			}
		}
	}

	// ---- 遠距離（弾） → 敵死亡 ----
	for (auto& b : bullets_) {
		if (!b || !b->IsActive()) {
			continue;
		}

		const AABB ba = b->GetAABB();
		for (Enemy* enemy : enemies_) {
			if (!enemy || enemy->IsDead()) {
				continue;
			}
			if (IsCollision(ba, enemy->GetAABB())) {
				enemy->TakeDamage(9999);
				b->Deactivate();
				break;
			}
		}
	}

	// ダメージタイル衝突（4隅＋中心）
	{
		Vector3 pts[5] = {
		    playerAabb.min,
		    {playerAabb.max.x,                             playerAabb.min.y,                             playerAabb.min.z                            },
		    {playerAabb.min.x,                             playerAabb.max.y,                             playerAabb.min.z                            },
		    playerAabb.max,
		    {(playerAabb.min.x + playerAabb.max.x) * 0.5f, (playerAabb.min.y + playerAabb.max.y) * 0.5f, (playerAabb.min.z + playerAabb.max.z) * 0.5f}
        };

		// 重複タイルを避けるための簡易集合（最大5個なのでベタでOK）
		IndexSet ids[5]{};
		int idCount = 0;

		auto addUnique = [&](IndexSet v) {
			for (int i = 0; i < idCount; ++i) {
				if (ids[i].xIndex == v.xIndex && ids[i].yIndex == v.yIndex)
					return;
			}
			ids[idCount++] = v;
		};

		for (int i = 0; i < 5; ++i) {
			IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(pts[i]);
			// 範囲外は GetMapChipTypeByIndex が blank を返す想定だけど、
			// ここで弾くなら弾いてもOK
			addUnique(idx);
		}

		for (int i = 0; i < idCount; ++i) {
			const uint32_t x = ids[i].xIndex;
			const uint32_t y = ids[i].yIndex;

			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kDamage) {
				continue;
			}

			Rect r = mapChipField_->GetRectByIndex(x, y);
			AABB tileAabb = MakeTileAabbFromRect(r);

			if (IsCollision(playerAabb, tileAabb)) {
				player_->OnCollision(nullptr);
				return;
			}
		}
	}
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		if (player_->IsDead()) {
			// 死亡演出フェーズに切り替え
			phase_ = Phase::kDeath;
			// 自キャラの座標を取得
			// const Vector3& deathParticlesPosition = player_->GetWorldPosition();
		}
		break;
	case Phase::kDeath:
		break;
	default:
		break;
	}
}

void GameScene::UpdateBullets_() {
	for (auto& b : bullets_) {
		if (b) {
			b->Update();
		}
	}

	// 非アクティブ除去
	bullets_.erase(
	    std::remove_if(bullets_.begin(), bullets_.end(), [](const std::unique_ptr<PlayerBullet>& b) { return !b || !b->IsActive(); }),
	    bullets_.end());
}

void GameScene::RemoveDeadEnemies_() {
	for (auto it = enemies_.begin(); it != enemies_.end();) {
		Enemy* e = *it;
		if (e && e->IsDeathEffectFinished()) {
			delete e;
			it = enemies_.erase(it);
			continue;
		}
		++it;
	}
}

bool GameScene::TryGetShootTargetPos_(Vector3& outPos) const {
	if (!player_) {
		return false;
	}

	const Vector3 playerPos = player_->GetWorldTransform().translation_;

	float bestDist2 = FLT_MAX;
	bool found = false;

	for (Enemy* e : enemies_) {
		if (!e || e->IsDead()) {
			continue;
		}

		const AABB a = e->GetAABB();
		const Vector3 center = {(a.min.x + a.max.x) * 0.5f, (a.min.y + a.max.y) * 0.5f, (a.min.z + a.max.z) * 0.5f};

		const float dx = center.x - playerPos.x;
		const float dy = center.y - playerPos.y;
		const float dz = center.z - playerPos.z;
		const float d2 = dx * dx + dy * dy + dz * dz;

		if (d2 < bestDist2) {
			bestDist2 = d2;
			outPos = center;
			found = true;
		}
	}

	return found;
}