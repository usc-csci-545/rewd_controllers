#ifndef REWD_CONTROLLERS_JOINTGROUPPOSITIONCONTROLLER_HPP_
#define REWD_CONTROLLERS_JOINTGROUPPOSITIONCONTROLLER_HPP_

#include <memory>
#include <ros/node_handle.h>
#include <actionlib/server/action_server.h>
#include <control_msgs/FollowJointTrajectoryAction.h>
#include <controller_interface/multi_interface_controller.h>
#include <realtime_tools/realtime_buffer.h>
#include <realtime_tools/realtime_server_goal_handle.h>
#include <dart/dynamics/dynamics.hpp>
#include "helpers.hpp"

namespace rewd_controllers {

class JointTrajectoryController
  : public controller_interface::MultiInterfaceController<
      hardware_interface::PositionJointInterface,
      hardware_interface::VelocityJointInterface,
      hardware_interface::EffortJointInterface,
      hardware_interface::JointStateInterface>
{
public:
  using Action = control_msgs::FollowJointTrajectoryAction;
  using ActionServer = actionlib::ActionServer<Action>;
  using GoalHandle = ActionServer::GoalHandle;

  using Feedback = control_msgs::FollowJointTrajectoryFeedback;
  using Result = control_msgs::FollowJointTrajectoryResult;

  JointTrajectoryController();
  virtual ~JointTrajectoryController();

  /** \brief The init function is called to initialize the controller from a
   * non-realtime thread with a pointer to the hardware interface, itself,
   * instead of a pointer to a RobotHW.
   *
   * \param robot The specific hardware interface used by this controller.
   *
   * \param n A NodeHandle in the namespace from which the controller
   * should read its configuration, and where it should set up its ROS
   * interface.
   *
   * \returns True if initialization was successful and the controller
   * is ready to be started.
   */
  bool init(hardware_interface::RobotHW* robot, ros::NodeHandle& n) override;

  /** \brief This is called from within the realtime thread just before the
   * first call to \ref update
   *
   * \param time The current time
   */
  void starting(const ros::Time& time) override;
  void stopping(const ros::Time& time) override;

  /*!
   * \brief Issues commands to the joint. Should be called at regular intervals
   */
  void update(const ros::Time& time, const ros::Duration& period) override;

private:
  struct TrajectoryContext
  {
    ros::Time mStartTime;
    std::shared_ptr<aikido::trajectory::Trajectory> mTrajectory;
    GoalHandle mGoalHandle;
  };

  void goalCallback(GoalHandle goalHandle);
  void cancelCallback(GoalHandle goalHandle);
  void nonRealtimeCallback(const ros::TimerEvent &event);

  JointAdapterFactory mAdapterFactory;
  dart::dynamics::SkeletonPtr mSkeleton;
  dart::dynamics::MetaSkeletonPtr mControlledSkeleton;
  std::shared_ptr<aikido::statespace::dart::MetaSkeletonStateSpace> mControlledSpace;

  std::unique_ptr<SkeletonJointStateUpdater> mSkeletonUpdater;
  std::vector<std::unique_ptr<JointAdapter>> mAdapters;
  Eigen::VectorXd mDesiredPosition;
  Eigen::VectorXd mDesiredVelocity;
  Eigen::VectorXd mDesiredAcceleration;
  Eigen::VectorXd mDesiredEffort;
  Eigen::VectorXd mActualPosition;
  Eigen::VectorXd mActualVelocity;
  Eigen::VectorXd mActualEffort;

  std::unique_ptr<ActionServer> mActionServer;
  ros::Timer mNonRealtimeTimer;

  // It would be better to use std::atomic<std::shared_ptr<T>> here. However,
  // this is not fully implemented in GCC 4.8.4, shippe with Ubuntu 14.04.
  realtime_tools::RealtimeBox<
    std::shared_ptr<TrajectoryContext>> mCurrentTrajectory;
};

} // namespace rewd_controllers

#endif // ifndef REWD_CONTROLLERS_JOINTGROUPPOSITIONCONTROLLER_HPP_