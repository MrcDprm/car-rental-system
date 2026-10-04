# Car Rental System

**English** | [Türkçe](README.tr.md)

A desktop management app for a small car rental office, written in C++20 and Qt 6: fleet, customers, reservations without double bookings, pick-up and return, maintenance, printable contracts and revenue reports. Data is stored locally in SQLite.

> 🚧 Work in progress. This README is the project plan and will be completed at v1.0.0.

## Plan

### MVP
- **Dashboard:** vehicles out, returns due today, overdue rentals, cars waiting for maintenance.
- **Fleet:** plate, brand, model, year, class (economy, compact, SUV, van…), transmission, fuel, seats, daily price, mileage, status (available, rented, maintenance). Search and filters.
- **Customers:** name, phone, e-mail, ID number, driving licence number and date. Checks: minimum age 21 and at least 2 years of licence.
- **Reservations:** pick a date range and see only the cars free in that range; overlapping bookings are impossible. Price = days × daily price, with weekly and monthly discounts and an optional deposit.
- **Pick-up and return:** mileage out and in, fuel level, late return fee, extra kilometres; the car's status and mileage update automatically. Cancelling a reservation.
- **Maintenance:** send a car to maintenance and back, record the cost; warning when the next service mileage is near.
- **Contract and receipt:** printable / PDF rental contract at pick-up and receipt at return.
- **Reports:** monthly revenue chart, most rented cars, fleet utilisation.
- **CSV export:** fleet, customers and rentals, ready to open in Excel.
- **Safety:** every input is validated, SQL queries use parameters only, money is stored in kuruş (integers) to avoid rounding errors.
- **Desktop app:** dark and light theme, Turkish and English, icon, version, About window, data in the user's folder, Windows installer (windeployqt + Inno Setup).
- **Tests:** pricing, date overlap, age and licence rules, fees, with Qt Test.

### Future Plans
- Staff accounts with roles.
- Online booking form.
- Damage photos at pick-up and return.
- Multiple branches.

## Tech Stack
- C++20, Qt 6 (Widgets, SQL, PrintSupport, Test)
- SQLite
- CMake, MinGW-w64 (MSYS2 UCRT64)
- windeployqt, Inno Setup
