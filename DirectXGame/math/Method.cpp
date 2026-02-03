#include "Method.h"
#include <cmath>
#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>

using namespace KamataEngine;

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
	Matrix4x4 result;
	result = {width / 2.0f, 0, 0, 0, 0, -height / 2.0f, 0, 0, 0, 0, maxDepth - minDepth, 0, left + (width / 2.0f), top + (height / 2.0f), minDepth, 1};
	return result;
}

Matrix4x4 MatrixMultiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;
	result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0];
	result.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1];
	result.m[0][2] = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2];
	result.m[0][3] = m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3];
	result.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0];
	result.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1];
	result.m[1][2] = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2];
	result.m[1][3] = m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3];
	result.m[2][0] = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0];
	result.m[2][1] = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1];
	result.m[2][2] = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2];
	result.m[2][3] = m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3];
	result.m[3][0] = m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0];
	result.m[3][1] = m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1];
	result.m[3][2] = m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2];
	result.m[3][3] = m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3];
	return result;
}

Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
	//====================
	// 拡縮の行列の作成
	//====================
	Matrix4x4 scaleMatrix4x4;
	scaleMatrix4x4.m[0][0] = scale.x;
	scaleMatrix4x4.m[0][1] = 0.0f;
	scaleMatrix4x4.m[0][2] = 0.0f;
	scaleMatrix4x4.m[0][3] = 0.0f;

	scaleMatrix4x4.m[1][0] = 0.0f;
	scaleMatrix4x4.m[1][1] = scale.y;
	scaleMatrix4x4.m[1][2] = 0.0f;
	scaleMatrix4x4.m[1][3] = 0.0f;

	scaleMatrix4x4.m[2][0] = 0.0f;
	scaleMatrix4x4.m[2][1] = 0.0f;
	scaleMatrix4x4.m[2][2] = scale.z;
	scaleMatrix4x4.m[2][3] = 0.0f;

	scaleMatrix4x4.m[3][0] = 0.0f;
	scaleMatrix4x4.m[3][1] = 0.0f;
	scaleMatrix4x4.m[3][2] = 0.0f;
	scaleMatrix4x4.m[3][3] = 1.0f;

	//===================
	// 回転の行列の作成
	//===================
	// Xの回転行列
	Matrix4x4 rotateMatrixX;
	rotateMatrixX.m[0][0] = 1.0f;
	rotateMatrixX.m[0][1] = 0.0f;
	rotateMatrixX.m[0][2] = 0.0f;
	rotateMatrixX.m[0][3] = 0.0f;

	rotateMatrixX.m[1][0] = 0.0f;
	rotateMatrixX.m[1][1] = cosf(rotate.x);
	rotateMatrixX.m[1][2] = sinf(rotate.x);
	rotateMatrixX.m[1][3] = 0.0f;

	rotateMatrixX.m[2][0] = 0.0f;
	rotateMatrixX.m[2][1] = -sinf(rotate.x);
	rotateMatrixX.m[2][2] = cosf(rotate.x);
	rotateMatrixX.m[2][3] = 0.0f;

	rotateMatrixX.m[3][0] = 0.0f;
	rotateMatrixX.m[3][1] = 0.0f;
	rotateMatrixX.m[3][2] = 0.0f;
	rotateMatrixX.m[3][3] = 1.0f;

	// Yの回転行列
	Matrix4x4 rotateMatrixY;
	rotateMatrixY.m[0][0] = cosf(rotate.y);
	rotateMatrixY.m[0][1] = 0.0f;
	rotateMatrixY.m[0][2] = -sinf(rotate.y);
	rotateMatrixY.m[0][3] = 0.0f;

	rotateMatrixY.m[1][0] = 0.0f;
	rotateMatrixY.m[1][1] = 1.0f;
	rotateMatrixY.m[1][2] = 0.0f;
	rotateMatrixY.m[1][3] = 0.0f;

	rotateMatrixY.m[2][0] = sinf(rotate.y);
	rotateMatrixY.m[2][1] = 0.0f;
	rotateMatrixY.m[2][2] = cosf(rotate.y);
	rotateMatrixY.m[2][3] = 0.0f;

	rotateMatrixY.m[3][0] = 0.0f;
	rotateMatrixY.m[3][1] = 0.0f;
	rotateMatrixY.m[3][2] = 0.0f;
	rotateMatrixY.m[3][3] = 1.0f;

	// Zの回転行列
	Matrix4x4 rotateMatrixZ;
	rotateMatrixZ.m[0][0] = cosf(rotate.z);
	rotateMatrixZ.m[0][1] = sinf(rotate.z);
	rotateMatrixZ.m[0][2] = 0.0f;
	rotateMatrixZ.m[0][3] = 0.0f;

	rotateMatrixZ.m[1][0] = -sinf(rotate.z);
	rotateMatrixZ.m[1][1] = cosf(rotate.z);
	rotateMatrixZ.m[1][2] = 0.0f;
	rotateMatrixZ.m[1][3] = 0.0f;

	rotateMatrixZ.m[2][0] = 0.0f;
	rotateMatrixZ.m[2][1] = 0.0f;
	rotateMatrixZ.m[2][2] = 1.0f;
	rotateMatrixZ.m[2][3] = 0.0f;

	rotateMatrixZ.m[3][0] = 0.0f;
	rotateMatrixZ.m[3][1] = 0.0f;
	rotateMatrixZ.m[3][2] = 0.0f;
	rotateMatrixZ.m[3][3] = 1.0f;

	// 回転行列の作成
	Matrix4x4 rotateMatrix4x4;

	rotateMatrix4x4 = MatrixMultiply(rotateMatrixX, MatrixMultiply(rotateMatrixY, rotateMatrixZ));

	//==================
	// 移動の行列の作成
	//==================
	Matrix4x4 translateMatrix4x4;
	translateMatrix4x4.m[0][0] = 1.0f;
	translateMatrix4x4.m[0][1] = 0.0f;
	translateMatrix4x4.m[0][2] = 0.0f;
	translateMatrix4x4.m[0][3] = 0.0f;

	translateMatrix4x4.m[1][0] = 0.0f;
	translateMatrix4x4.m[1][1] = 1.0f;
	translateMatrix4x4.m[1][2] = 0.0f;
	translateMatrix4x4.m[1][3] = 0.0f;

	translateMatrix4x4.m[2][0] = 0.0f;
	translateMatrix4x4.m[2][1] = 0.0f;
	translateMatrix4x4.m[2][2] = 1.0f;
	translateMatrix4x4.m[2][3] = 0.0f;

	translateMatrix4x4.m[3][0] = translate.x;
	translateMatrix4x4.m[3][1] = translate.y;
	translateMatrix4x4.m[3][2] = translate.z;
	translateMatrix4x4.m[3][3] = 1.0f;

	//====================
	// アフィン行列の作成
	//====================
	// 上で作った行列からアフィン行列を作る
	// アフィン行列の作成（スケール→回転→移動の順）
	Matrix4x4 affineMatrix4x4;
	affineMatrix4x4 = MatrixMultiply(scaleMatrix4x4, MatrixMultiply(rotateMatrix4x4, translateMatrix4x4));

	return affineMatrix4x4;
}

Matrix4x4 Inverse(const Matrix4x4& m) {
	float determinant;
	determinant =
	    m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1] -
	    m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1] -
	    m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2] +
	    m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1] -
	    m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0] -
	    m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0] + m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0] + m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0];
	Matrix4x4 result;
	result = {
	    (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][3] * m.m[2][2] * m.m[3][1] - m.m[1][2] * m.m[2][1] * m.m[3][3] -
	     m.m[1][1] * m.m[2][3] * m.m[3][2]) /
	        determinant,
	    (-m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[2][1] * m.m[3][3] +
	     m.m[0][1] * m.m[2][3] * m.m[3][2]) /
	        determinant,
	    (m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[3][1] - m.m[0][2] * m.m[1][1] * m.m[3][3] -
	     m.m[0][1] * m.m[1][3] * m.m[3][2]) /
	        determinant,
	    (-m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][3] * m.m[1][2] * m.m[2][1] + m.m[0][2] * m.m[1][1] * m.m[2][3] +
	     m.m[0][1] * m.m[1][3] * m.m[2][2]) /
	        determinant,
	    (-m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][3] * m.m[2][2] * m.m[3][0] + m.m[1][2] * m.m[2][0] * m.m[3][3] +
	     m.m[1][0] * m.m[2][3] * m.m[3][2]) /
	        determinant,
	    (m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] + m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[2][2] * m.m[3][0] - m.m[0][2] * m.m[2][0] * m.m[3][3] -
	     m.m[0][0] * m.m[2][3] * m.m[3][2]) /
	        determinant,
	    (-m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][3] * m.m[1][2] * m.m[3][0] + m.m[0][2] * m.m[1][0] * m.m[3][3] +
	     m.m[0][0] * m.m[1][3] * m.m[3][2]) /
	        determinant,
	    (m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] + m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] - m.m[0][2] * m.m[1][0] * m.m[2][3] -
	     m.m[0][0] * m.m[1][3] * m.m[2][2]) /
	        determinant,
	    (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][3] * m.m[2][1] * m.m[3][0] - m.m[1][1] * m.m[2][0] * m.m[3][3] -
	     m.m[1][0] * m.m[2][3] * m.m[3][1]) /
	        determinant,
	    (-m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[2][1] * m.m[3][0] + m.m[0][1] * m.m[2][0] * m.m[3][3] +
	     m.m[0][0] * m.m[2][3] * m.m[3][1]) /
	        determinant,
	    (m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] + m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][3] * m.m[1][1] * m.m[3][0] - m.m[0][1] * m.m[1][0] * m.m[3][3] -
	     m.m[0][0] * m.m[1][3] * m.m[3][1]) /
	        determinant,
	    (-m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] + m.m[0][1] * m.m[1][0] * m.m[2][3] +
	     m.m[0][0] * m.m[1][3] * m.m[2][1]) /
	        determinant,
	    (-m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][2] * m.m[2][1] * m.m[3][0] + m.m[1][1] * m.m[2][0] * m.m[3][2] +
	     m.m[1][0] * m.m[2][2] * m.m[3][1]) /
	        determinant,
	    (m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] + m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][2] * m.m[2][1] * m.m[3][0] - m.m[0][1] * m.m[2][0] * m.m[3][2] -
	     m.m[0][0] * m.m[2][2] * m.m[3][1]) /
	        determinant,
	    (-m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][2] * m.m[1][1] * m.m[3][0] + m.m[0][1] * m.m[1][0] * m.m[3][2] +
	     m.m[0][0] * m.m[1][2] * m.m[3][1]) /
	        determinant,
	    (m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] + m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][2] * m.m[1][1] * m.m[2][0] - m.m[0][1] * m.m[1][0] * m.m[2][2] -
	     m.m[0][0] * m.m[1][2] * m.m[2][1]) /
	        determinant};
	return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	Matrix4x4 result;
	result = {(1.0f / aspectRatio) * (1.0f / std::tanf(fovY / 2.0f)), 0, 0, 0, 0, (1.0f / std::tanf(fovY / 2.0f)), 0, 0, 0, 0, farClip / (farClip - nearClip), 1, 0, 0,
	          (-nearClip * farClip) / (farClip - nearClip),           0};
	return result;
}

Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t) {
	Vector3 point1;
	Vector3 point2;
	point1 = {t * v1.x, t * v1.y, t * v1.z};
	point2 = {(1.0f - t) * v2.x, (1.0f - t) * v2.y, (1.0f - t) * v2.z};
	return {point1.x + point2.x, point1.y + point2.y, point1.z + point2.z};
}

float EaseInOut(float t) {
	// 0.0〜1.0の範囲に制限
	if (t < 0.0f) {
		t = 0.0f;
	} else if (t > 1.0f) {
		t = 1.0f;
	}

	float result = 0.0f;
	if (t < 0.5f) {
		result = 4.0f * t * t * t;
	} else {
		float temp = -2.0f * t + 2.0f;
		result = 1.0f - (temp * temp * temp) / 2.0f;
	}

	return result;
}

bool IsCollision(const AABB& a, const AABB& b) { return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y); }

Matrix4x4 MakeRotateXMatrix(float radian) {
	Matrix4x4 result;
	result = {1, 0, 0, 0, 0, std::cosf(radian), std::sinf(radian), 0, 0, -std::sinf(radian), std::cosf(radian), 0, 0, 0, 0, 1};

	return result;
}

Matrix4x4 MakeRotateYMatrix(float radian) {
	Matrix4x4 result;
	result = {std::cosf(radian), 0, -std::sinf(radian), 0, 0, 1, 0, 0, std::sinf(radian), 0, std::cosf(radian), 0, 0, 0, 0, 1};

	return result;
}

Matrix4x4 MakeRotateZMatrix(float radian) {
	Matrix4x4 result;
	result = {std::cosf(radian), std::sinf(radian), 0, 0, -std::sinf(radian), std::cosf(radian), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

	return result;
}

Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix) {
	Vector3 result;
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	result.z /= w;
	return result;
}

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
	return {
	    v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
	    v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
	    v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2],
	};
}

float Length(const Vector3& v) {
	float result;
	result = {sqrtf(powf(v.x, 2) + powf(v.y, 2) + powf(v.z, 2))};
	return result;
}

Vector3 Normalize(const Vector3& v) {
	float len = Length(v);
	return {v.x / len, v.y / len, v.z / len};
}

bool IntersectAABB(const AABB& a, const AABB& b) {
	// 1軸でも重なっていなければfalse
	if (a.max.x < b.min.x || a.min.x > b.max.x)
		return false;
	if (a.max.y < b.min.y || a.min.y > b.max.y)
		return false;
	if (a.max.z < b.min.z || a.min.z > b.max.z)
		return false;
	return true;
}