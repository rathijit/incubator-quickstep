/**
 *   Copyright 2016, Quickstep Research Group, Computer Sciences Department,
 *     University of Wisconsin—Madison.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 **/

#ifndef QUICKSTEP_QUERY_EXECUTION_POLICY_ENFORCER_HPP_
#define QUICKSTEP_QUERY_EXECUTION_POLICY_ENFORCER_HPP_

#include <cstddef>
#include <memory>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "query_execution/QueryExecutionTypedefs.hpp"
#include "query_execution/QueryManager.hpp"
#include "query_execution/WorkerMessage.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

#include "tmb/id_typedefs.h"
#include "tmb/message_bus.h"
#include "tmb/tagged_message.h"

namespace quickstep {

class CatalogDatabaseLite;
class QueryHandle;
class StorageManager;
class WorkerDirectory;

/**
 * @brief A class that ensures that a high level policy is maintained
 *        in sharing resources among concurrent queries.
 **/
class PolicyEnforcer {
 public:
  /**
   * @brief Constructor.
   *
   * @param foreman_client_id The TMB client ID of the Foreman.
   * @param num_numa_nodes Number of NUMA nodes used by the system.
   * @param catalog_database The CatalogDatabase used.
   * @param storage_manager The StorageManager used.
   * @param bus The TMB.
   **/
  PolicyEnforcer(const tmb::client_id foreman_client_id,
                 const std::size_t num_numa_nodes,
                 CatalogDatabaseLite *catalog_database,
                 StorageManager *storage_manager,
                 WorkerDirectory *worker_directory,
                 tmb::MessageBus *bus,
                 const bool profile_individual_workorders = false)
      : foreman_client_id_(foreman_client_id),
        num_numa_nodes_(num_numa_nodes),
        catalog_database_(catalog_database),
        storage_manager_(storage_manager),
        worker_directory_(worker_directory),
        bus_(bus),
        profile_individual_workorders_(profile_individual_workorders) {}

  /**
   * @brief Destructor.
   **/
  ~PolicyEnforcer() {
    if (hasQueries()) {
      LOG(WARNING) << "Destructing PolicyEnforcer with some unfinished or "
                      "waiting queries";
    }
  }

  /**
   * @brief Admit a query to the system.
   *
   * @param query_handle The QueryHandle for the new query.
   *
   * @return Whether the query was admitted to the system.
   **/
  bool admitQuery(QueryHandle *query_handle);

  /**
   * @brief Admit multiple queries in the system.
   *
   * @note In the current simple implementation, we only allow one active
   *       query in the system. Other queries will have to wait.
   *
   * @param query_handles A vector of QueryHandles for the queries to be
   *        admitted.
   *
   * @return True if all the queries were admitted, false if at least one query
   *         was not admitted.
   **/
  bool admitQueries(const std::vector<QueryHandle*> &query_handles);

  /**
   * @brief Remove a given query that is under execution.
   *
   * @note This function is made public so that it is possible for a query
   *       to be killed. Otherwise, it should only be used privately by the
   *       class.
   *
   * TODO(harshad) - Extend this function to support removal of waiting queries.
   *
   * @param query_id The ID of the query to be removed.
   **/
  void removeQuery(const std::size_t query_id);

  /**
   * @brief Get worker messages to be dispatched. These worker messages come
   *        from the active queries.
   *
   * @param worker_messages The worker messages to be dispatched.
   **/
  void getWorkerMessages(
      std::vector<std::unique_ptr<WorkerMessage>> *worker_messages);

  /**
   * @brief Process a message sent to the Foreman, which gets passed on to the
   *        policy enforcer.
   *
   * @param message The message.
   **/
  void processMessage(const TaggedMessage &tagged_message);

  /**
   * @brief Check if there are any queries to be executed.
   *
   * @return True if there is at least one active or waiting query, false if
   *         the policy enforcer doesn't have any query.
   **/
  inline bool hasQueries() const {
    return !(admitted_queries_.empty() && waiting_queries_.empty());
  }

  /**
   * @brief Get the profiling results for individual work order execution for a
   *        given query.
   *
   * @note This function should only be called if profiling individual work
   *       orders option is enabled.
   *
   * @param query_id The ID of the query for which the profiling results are
   *        requested.
   *
   * @return A vector of tuples, each being a single profiling entry.
   **/
  inline const std::vector<std::tuple<std::size_t, std::size_t, std::size_t>>&
      getProfilingResults(const std::size_t query_id) const {
    DCHECK(profile_individual_workorders_);
    DCHECK(workorder_time_recorder_.find(query_id) !=
           workorder_time_recorder_.end());
    return workorder_time_recorder_.at(query_id);
  }

 private:
  static constexpr std::size_t kMaxConcurrentQueries = 1;

  /**
   * @brief Record the execution time for a finished WorkOrder.
   *
   * TODO(harshad) - Extend the functionality to rebuild work orders.
   *
   * @param proto The completion message proto sent after the WorkOrder
   *        execution.
   **/
  void recordTimeForWorkOrder(
      const serialization::NormalWorkOrderCompletionMessage &proto);

  const tmb::client_id foreman_client_id_;
  const std::size_t num_numa_nodes_;

  CatalogDatabaseLite *catalog_database_;
  StorageManager *storage_manager_;
  WorkerDirectory *worker_directory_;

  tmb::MessageBus *bus_;
  const bool profile_individual_workorders_;

  // Key = query ID, value = QueryManager* for the key query.
  std::unordered_map<std::size_t, std::unique_ptr<QueryManager>> admitted_queries_;

  // The queries which haven't been admitted yet.
  std::queue<QueryHandle*> waiting_queries_;

  // Key = Query ID.
  // Value = A tuple indicating a record of executing a work order.
  // Within a tuple ...
  // 1st element: Logical worker ID.
  // 2nd element: Operator ID.
  // 3rd element: Time in microseconds to execute the work order.
  std::unordered_map<
      std::size_t,
      std::vector<std::tuple<std::size_t, std::size_t, std::size_t>>>
      workorder_time_recorder_;

  DISALLOW_COPY_AND_ASSIGN(PolicyEnforcer);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_QUERY_EXECUTION_QUERY_MANAGER_HPP_
