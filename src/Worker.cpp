#include "Worker.hpp"
#include <iostream>

namespace Server {

void CaesarWorker::process_unit(std::shared_ptr<Unit> unit) {
    std::cout << "Обработка юнита: [" << unit->get_start() << ", " << unit->get_end() << "]" << std::endl;
    // ... дешифровка
    unit->mark_as_done();  // Помечаем юнит как завершенный
}

} // namespace Server
