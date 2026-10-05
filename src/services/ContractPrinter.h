#pragma once

#include "core/Models.h"

// Kiralama sözleşmesi (teslimde) ve iade fişi (iadede) PDF olarak.
namespace ContractPrinter {

QString html(const Rental &rental, const Vehicle &vehicle, const Customer &customer);
bool savePdf(const QString &html, const QString &path);

} // namespace ContractPrinter