#include <ClientSession.hpp>
#include <CoordServ.hpp>
#include <algorithm>
#include <asio/co_spawn.hpp>
#include <iostream>

namespace server {

ClientSession::ClientSession(
    asio::ip::tcp::socket socket,
    CoordinatorServer &server
)
    : m_conn(std::move(socket)), m_server(server) {
}

void ClientSession::start() {
    asio::co_spawn(m_conn.get_executor(), read_loop(), asio::detached);
}

void ClientSession::enqueue_snapshot(ProgressSnapshot snap) {
    if (snap.finished) {
        std::erase_if(m_send_queue, [](const ProgressSnapshot &s) {
            return !s.finished;
        });
        m_send_queue.push_back(std::move(snap));
    } else {
        std::erase_if(m_send_queue, [](const ProgressSnapshot &s) {
            return !s.finished;
        });
        m_send_queue.push_back(std::move(snap));
    }
    pump_send_queue();
}

void ClientSession::pump_send_queue() {
    if (m_send_pump_active || m_send_queue.empty()) {
        return;
    }
    m_send_pump_active = true;
    auto self = shared_from_this();
    asio::co_spawn(
        m_conn.get_executor(),
        [self]() -> asio::awaitable<void> {
            try {
                while (!self->m_send_queue.empty()) {
                    ProgressSnapshot snap = std::move(self->m_send_queue.front());
                    self->m_send_queue.pop_front();
                    co_await self->send_progress_snapshot(snap);
                }
            } catch (const std::exception &e) {
                if (self->m_conn.is_open()) {
                    std::cerr << "Client progress send pump error: " << e.what()
                              << std::endl;
                }
                self->m_send_queue.clear();
            }
            self->m_send_pump_active = false;
            if (!self->m_send_queue.empty()) {
                self->pump_send_queue();
            }
        },
        asio::detached
    );
}

asio::awaitable<void> ClientSession::send_progress_snapshot(
    const ProgressSnapshot &snap
) {
    if (!m_conn.is_open()) {
        co_return;
    }
    auto payload = std::make_unique<json_protocol::StatusPayload>(
        m_current_cipher,
        snap.best.text_,
        snap.best.key_,
        snap.best.score_
    );
    payload->set_progress(snap.progress_percent);
    payload->set_done_units(snap.done_units);
    payload->set_total_units(snap.total_units);
    payload->set_leased_units(snap.leased_units);
    payload->set_workers(snap.workers);
    payload->set_finished(snap.finished);
    auto msg =
        json_protocol::Message::create_status_response(std::move(payload));
    co_await m_conn.send_message(msg);
}

void ClientSession::post_progress_update(
    const Result &best,
    int progress_percent,
    std::size_t done_units,
    std::size_t total_units,
    std::size_t leased_units,
    std::size_t workers,
    bool finished
) {
    if (!m_conn.is_open()) {
        return;
    }
    ProgressSnapshot snap;
    snap.best = best;
    snap.progress_percent = progress_percent;
    snap.done_units = done_units;
    snap.total_units = total_units;
    snap.leased_units = leased_units;
    snap.workers = workers;
    snap.finished = finished;
    enqueue_snapshot(std::move(snap));
}

void ClientSession::post_final_result(const Result &result) {
    if (!m_server.m_coordinator) {
        return;
    }
    post_progress_update(
        result,
        100,
        m_server.m_coordinator->done_units_count(),
        m_server.m_coordinator->unit_count(),
        0,
        m_server.m_workers.size(),
        true
    );
}

asio::awaitable<void> ClientSession::read_loop() {
    try {
        auto msg = co_await m_conn.read_message();
        co_await handle_task_request(msg);
    } catch (const std::exception &e) {
        std::cerr << "Client session error: " << e.what() << std::endl;
    }
}

}  // namespace server
