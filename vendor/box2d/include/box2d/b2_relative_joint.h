// MIT License

// Copyright (c) 2019 Erin Catto
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef B2_RELATIVE_JOINT_H
#define B2_RELATIVE_JOINT_H

#include "b2_joint.h"

/// Retail Sound Shapes' RELATIVE joint. Its verified velocity-solver callback
/// transfers each change in body A's linear velocity to body B. The retail
/// setRelativeVel vector is retained as runtime state; its further solver use
/// has not yet been established.
struct B2_API b2RelativeJointDef : public b2JointDef
{
	b2RelativeJointDef()
	{
		type = e_relativeJoint;
		relativeVelocity.SetZero();
	}

	b2Vec2 relativeVelocity;
};

class B2_API b2RelativeJoint : public b2Joint
{
public:
	b2Vec2 GetAnchorA() const override;
	b2Vec2 GetAnchorB() const override;
	b2Vec2 GetReactionForce(float inv_dt) const override;
	float GetReactionTorque(float inv_dt) const override;

	void SetRelativeVelocity(const b2Vec2& velocity);
	const b2Vec2& GetRelativeVelocity() const;

protected:
	friend class b2Joint;
	explicit b2RelativeJoint(const b2RelativeJointDef* def);
	void InitVelocityConstraints(const b2SolverData& data) override;
	void SolveVelocityConstraints(const b2SolverData& data) override;
	bool SolvePositionConstraints(const b2SolverData& data) override;

private:
	b2Vec2 m_relativeVelocity;
	b2Vec2 m_previousLinearVelocityA;
};

#endif
