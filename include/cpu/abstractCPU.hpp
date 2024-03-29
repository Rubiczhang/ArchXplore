#pragma once

#include <atomic>

#include "sparta/simulation/Unit.hpp"
#include "sparta/utils/SpartaAssert.hpp"
#include "sparta/simulation/Clock.hpp"
#include "sparta/statistics/Counter.hpp"

#include "iss/abstractISS.hpp"

// Forward Declaration
namespace archXplore::system
{
    class abstractSystem;
};

namespace archXplore
{

    namespace cpu
    {

        enum cpuStatus_t
        {
            INACTIVE,
            ACTIVE,
            BLOCKED_COMM,
            BLOCKED_MUTEX,
            BLOCKED_BARRIER,
            BLOCKED_COND,
            BLOCKED_JOIN,
            COMPLETED,
            NUM_STATUSES
        };

        class abstractCPU : public sparta::Unit
        {
        public:
            // Delete copy-construct function
            abstractCPU(const abstractCPU &that) = delete;
            abstractCPU &operator=(const abstractCPU &that) = delete;

            abstractCPU(sparta::TreeNode *tn);

            ~abstractCPU();

            virtual auto reset() -> void = 0;

            auto startUpMonitor() -> void
            {
                // Wake up thread 0
                if(m_tid == 0) {
                    m_status = cpu::cpuStatus_t::ACTIVE;
                    this->reset();
                } else {
                    wakeUpMonitor();
                }
            }

            auto wakeUpMonitor() -> void
            {
                // TODO : Remove in the future, wakeUp monitor is leveraged to implement thread API like cond_wait, join, etc.
                //        Thread creation should be done by cpu 0 rather than itself.
                {
                    if(m_status == cpu::cpuStatus_t::INACTIVE && getISSPtr()->readyToPowerOn()) {
                        m_status = cpu::cpuStatus_t::ACTIVE;
                        this->reset();
                        return;
                    }
                    
                    if(m_status == cpu::cpuStatus_t::INACTIVE && getISSPtr()->readyToPowerOff()) {
                        return;
                    }
                }
                m_wakeup_monitor_event.schedule();
            };

            auto isRunning() const -> bool
            {
                return m_status > cpuStatus_t::INACTIVE && m_status < cpuStatus_t::COMPLETED;
            };
            auto isBlocked() const -> bool
            {
                return m_status > cpuStatus_t::ACTIVE && m_status < cpuStatus_t::COMPLETED;
            };
            auto isCompleted() const -> bool
            {
                return m_status == cpuStatus_t::COMPLETED;
            };
            auto getISSPtr() -> iss::abstractISS *
            {
                return m_iss.get();
            };
            auto getSystemPtr() -> system::abstractSystem *;
            auto getThreadID() -> const hartId_t
            {
                return m_tid;
            };
            auto setISS(iss::abstractISS::UniquePtr iss) -> void
            {
                sparta_assert((iss != nullptr), "Setting iss to nullptr");
                m_iss = std::move(iss);
                m_iss->setCPU(this);
            };

        public:
            // CPU Status
            cpuStatus_t m_status;
            // Cycle counter
            sparta::Counter m_cycle;
            // Instruction retired counter
            sparta::Counter m_instret;
            // Unique Thread Id
            hartId_t m_tid;
            // Processor frequency
            sparta::Clock::Frequency m_freq;

        private:
            // ISS Ptr
            iss::abstractISS::UniquePtr m_iss;
            sparta::Event<sparta::SchedulingPhase::Update> m_wakeup_monitor_event;
        };

    } // namespace iss
} // namespace archXplore
