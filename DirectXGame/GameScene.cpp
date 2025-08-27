#include "GameScene.h"

using namespace KamataEngine;

namespace {
constexpr float kFadeTimeSec = 1.0f;
constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;
} // namespace

//static inline bool IntersectAABB(const AABB& a, const AABB& b) {
//	return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
//}

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
	// ゴールの開放
	delete goal_;
	goal_ = nullptr;
	// 敵キャラの開放
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	// マップチップフィールドの開放
	delete mapChipField_;

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
}

void GameScene::Initialize() {

	// 3Dモデルデータの生成
	playerModel_ = Model::CreateFromOBJ("player", true);
	// 3Dモデルデータの生成
	enemyModel_ = Model::CreateFromOBJ("enemy", true);
	// 敵を複数生成
	const int enemyCount = 3;
	for (int32_t i = 0; i < enemyCount; ++i) {
		Enemy* newEnemy = new Enemy();
		Vector3 enemyPosition = {float(i + 7) * 7.0f, 1.0f, 0.0f}; // 一体ずつX方向にずらす
		newEnemy->Initialize(enemyModel_, &camera_, enemyPosition);
		enemies_.push_back(newEnemy);
	}
	particleModel_ = Model::CreateFromOBJ("particle", true);
	// ブロックモデルデータの生成
	modelBlock_ = Model::CreateFromOBJ("cube", true);
	// 天球モデルデータの生成
	modelSkydome_ = Model::CreateFromOBJ("sky_sphere", true);

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

	// 天球の生成
	skydome_ = new Skydome;
	// 天球の初期化
	skydome_->Initialize();

	GenetateBlocks();

	// ゲームプレイフェーズから開始
	phase_ = Phase::kPlay;

	Model* goalModel = Model::CreateFromOBJ("goal", true);
	; // まずは流用
	                                // 配置：固定 or CSVから検索（ここでは軽いCSV検索例：タイルID=9をゴール扱い）
	Vector3 goalPos = mapChipField_->GetMapChipPositionByIndex(90, 18);

	goal_ = new Goal();
	goal_->Initialize(goalModel, goalPos);

	// フェード
	fade_ = new Fade();
	fade_->Initialize();

	// ★ゲーム開始時は黒→透明のフェードイン。完了までは動かさない
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

		//float dt = 1.0f / 60.0f; // 実フレーム時間を持っているなら差し替え
		if (goal_)
			goal_->Update(/*dt*/);

		// 自キャラの更新
		player_->Update();
		// 天球の更新
		skydome_->Update();
		// 敵キャラの更新
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

		// すべての当たり判定を行う
		CheckAllCollisions();

		// 攻撃ヒット判定 & 敵の消滅・デス演出
		if (/* プレイヤーが攻撃Active中 */ player_->IsAttackHitboxActive()) {
			AABB atk = player_->GetAttackAABB();
			const Vector3 playerForward = {/* あなたの前方向計算に合わせる（+Xなら {1,0,0}） */ 1.0f, 0.0f, 0.0f};

			for (auto it = enemies_.begin(); it != enemies_.end(); /* no ++ here */) {
				Enemy* e = *it;
				if (!e) {
					it = enemies_.erase(it);
					continue;
				}

				if (IntersectAABB(atk, e->GetAABB())) {
					// ダメージ & ノックバック
					e->TakeDamage(1);
					e->ApplyKnockback(playerForward, 0.6f);

					if (e->IsDead()) {
						// ★敵デス演出：簡易パーティクルを出してから消す
						if (!deathParticles_) {
							const Vector3 pos = e->GetAABB().max; // ざっくり上面。厳密には中心が良い
							deathParticles_ = new DeathParticles;
							deathParticles_->Initialize(particleModel_, &camera_, pos);
						}
						// リストから除去
						it = enemies_.erase(it);
						delete e;
						continue;
					}
				}
				++it;
			}
		}

		// ★ゴール到達判定（通り抜けOKのトリガー）
		if (goal_ && goal_->IsActive() && IntersectAABB(player_->GetAABB(), goal_->GetAABB())) {
			goal_->SetActive(false);
			result_ = Result::kClear;
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec);
			phase_ = Phase::kFadeOut;
			break;
		}

		// ★死亡検知→フェードアウト開始（タイトルへ戻る準備）
		if (player_->IsDead()) {

			// 生成処理
			const Vector3& pos = player_->GetWorldPosition();
			deathParticles_ = new DeathParticles;
			deathParticles_->Initialize(particleModel_, &camera_, pos);
			phase_ = Phase::kDeath;
			return;
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
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		// フェードは最後に
		fade_->Draw();
		break;

	case Phase::kPlay:
		// 自キャラの描画
		player_->Draw();
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ★ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		break;

	case Phase::kDeath:
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}
		if (deathParticles_) {
			deathParticles_->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ★ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		break;

	case Phase::kFadeOut:
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ★ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		// フェードは最後に
		fade_->Draw();
		break;
	default:
		break;
	}
}

void GameScene::GenetateBlocks() {
	// 要素数
	const uint32_t kNumBlockVirtical = mapChipField_->GetNumBlockVertical();
	const uint32_t kNumBlockHorizontal = mapChipField_->GetNumBlockHorizontal();
	// ブロック一個分の縦横幅
	// const float kBlockWidth = 1.0f;
	// const float kBlockHeight = 1.0f;
	// 要素数を変更する
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		// 1列の要素数を設定（横方向のブロック数）
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	// キューブの生成
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}

void GameScene::CheckAllCollisions() {
#pragma region
	{
		// 判定対象1と2の座標
		AABB aabb1, aabb2;

		// 自キャラの座標
		aabb1 = player_->GetAABB();

		// 自キャラと敵球すべての当たり判定
		for (Enemy* enemy : enemies_) {
			// 敵弾の座標
			aabb2 = enemy->GetAABB();

			if (IsCollision(aabb1, aabb2)) {
				// 衝突応答処理
				// 自キャラの衝突時関数を呼び出す
				player_->OnCollision(enemy);
				// 敵の衝突時関数を呼び出す
				enemy->OnCollision(player_);
			}
		}
	}
#pragma endregion
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
