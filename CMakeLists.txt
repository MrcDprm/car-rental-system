cmake_minimum_required(VERSION 3.21)
project(CarRental VERSION 0.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql Widgets Test)
qt_standard_project_setup()

# İş kuralları ve veri katmanı: arayüzden bağımsız, testler de bunu kullanır
qt_add_library(car_core STATIC
    src/core/Models.h
    src/core/Money.cpp
    src/core/Pricing.cpp
    src/core/Rules.cpp
    src/data/Database.cpp
    src/data/VehicleRepository.cpp
    src/data/CustomerRepository.cpp
    src/data/RentalRepository.cpp
    src/data/MaintenanceRepository.cpp
    src/data/ReportRepository.cpp
    src/services/CsvExport.cpp
    src/services/DemoData.cpp
    src/app/I18n.cpp
    src/app/Settings.cpp
)
target_include_directories(car_core PUBLIC src)
target_link_libraries(car_core PUBLIC Qt6::Core Qt6::Sql)

# Sürüm numarası ve ikon .exe dosyasına buradan gömülür (Özellikler > Ayrıntılar)
configure_file(resources/app.rc.in ${CMAKE_BINARY_DIR}/app.rc @ONLY)

qt_add_executable(CarRental WIN32
    src/main.cpp
    src/app/Theme.cpp
    src/services/ContractPrinter.cpp
    src/ui/MainWindow.cpp
    src/ui/DashboardPage.cpp
    src/ui/FleetPage.cpp
    src/ui/VehicleDialog.cpp
    src/ui/CustomersPage.cpp
    src/ui/CustomerDialog.cpp
    src/ui/RentalsPage.cpp
    src/ui/RentalDialog.cpp
    src/ui/ReturnDialog.cpp
    src/ui/MaintenanceDialog.cpp
    src/ui/ReportsPage.cpp
    src/ui/RevenueChart.cpp
    src/ui/AboutDialog.cpp
    ${CMAKE_BINARY_DIR}/app.rc
)
qt_add_resources(CarRental "app_resources" PREFIX "/" BASE resources FILES resources/icon.png)
target_link_libraries(CarRental PRIVATE car_core Qt6::Widgets)
target_compile_definitions(CarRental PRIVATE APP_VERSION="${PROJECT_VERSION}")

enable_testing()
qt_add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE car_core Qt6::Test)
add_test(NAME test_core COMMAND test_core)
qt_add_executable(test_data tests/test_data.cpp)
target_link_libraries(test_data PRIVATE car_core Qt6::Test)
add_test(NAME test_data COMMAND test_data)
qt_add_executable(test_services tests/test_services.cpp src/services/ContractPrinter.cpp)
target_link_libraries(test_services PRIVATE car_core Qt6::Gui Qt6::Test)
add_test(NAME test_services COMMAND test_services)
