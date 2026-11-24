// Race: json (go)
// Extracted from suite/go/misc.go — build and run via suite/run.py

package main
import (
	"encoding/json"
	"fmt"
)

type Order struct {
	ID         int64    `json:"id"`
	CustomerID int64    `json:"customer_id"`
	Status     string   `json:"status"`
	TotalCents int64    `json:"total_cents"`
	Items      int64    `json:"items"`
	Tags       []string `json:"tags"`
}

type OrdersDoc struct {
	Orders []Order `json:"orders"`
}

type jsonResult struct {
	orders, totalCents, items int64
	out                       []byte
}

func jsonRoundtrip(text []byte) (jsonResult, error) {
	var doc OrdersDoc
	if err := json.Unmarshal(text, &doc); err != nil {
		return jsonResult{}, fmt.Errorf("parse orders: %w", err)
	}
	var total, items int64
	for _, o := range doc.Orders {
		total += o.TotalCents
		items += o.Items
	}
	out, err := json.Marshal(doc)
	if err != nil {
		return jsonResult{}, fmt.Errorf("serialise orders: %w", err)
	}
	return jsonResult{orders: int64(len(doc.Orders)), totalCents: total, items: items, out: out}, nil
}
