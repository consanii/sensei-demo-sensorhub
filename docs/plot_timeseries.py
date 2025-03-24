# ----------------------------------------------------------------------
#
# File: plot_timeseries.py
#
# Last edited: 23.04.2025
#
# Copyright (c) 2025 ETH Zurich and University of Bologna
#
# Authors:
# - Philip Wiese (wiesep@iis.ee.ethz.ch), ETH Zurich
#
# ----------------------------------------------------------------------
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import dash_bootstrap_components as dbc
import pandas as pd
import plotly.graph_objects as go
from dash import Dash, Input, Output, State, dcc, html

# Path to your data
csv_file_paths = [
    # 'SENSEI-DATA.log',
]

from pathlib import Path

import pandas as pd


class IncrementalDataLoader:

    def __init__(self, file_paths):
        self.file_paths = file_paths
        self.data_cache = {}  # Maps path -> dataframe
        self.file_positions = {}  # Maps path -> number of rows previously read
        self.merged_df = pd.DataFrame()

    def load_data(self):
        updated = False

        for path in self.file_paths:
            current_rows = self._count_rows(path)

            last_pos = self.file_positions.get(path, 1)
            if current_rows > last_pos:
                # New rows available
                new_df = pd.read_csv(path, comment = '[', skiprows = last_pos)

                # Parse datetime from filename
                filename = Path(path).stem
                date_str = '_'.join(filename.split('_')[-2:])
                date = pd.to_datetime(date_str, format = '%Y-%m-%d_%H-%M')

                # If datacache is not empty, use the column names from the first dataframe
                if path in self.data_cache:
                    new_df.columns = self.data_cache[path].columns

                new_df['BME688_Pressure'] *= 10
                new_df['Timestamp'] = date + pd.to_timedelta(new_df['Timestamp'], unit = 'ms')

                # Insert NaN row at the end
                # new_df = pd.concat(
                #     [new_df, pd.DataFrame({'Timestamp': [new_df['Timestamp'].iloc[-1] + pd.Timedelta(milliseconds=1)]})],
                #     ignore_index=True
                # )

                # Update internal state
                if path in self.data_cache:
                    self.data_cache[path] = pd.concat([self.data_cache[path], new_df], ignore_index = True)
                else:
                    self.data_cache[path] = new_df

                self.file_positions[path] = current_rows

                print(f"Loaded {current_rows - last_pos} new rows from {path}")
                updated = True

        if updated:
            # Recompute merged dataframe only if new data was added
            self.merged_df = pd.concat(self.data_cache.values(), ignore_index = True)
        else:
            print("No new data available.")

        return self.merged_df

    def _count_rows(self, path):
        with open(path) as f:
            return sum(1 for _ in f)


# Initialize app
app = Dash(__name__, external_stylesheets = [dbc.themes.BOOTSTRAP])
app.title = "Sensor Dashboard"

dataloader = IncrementalDataLoader(csv_file_paths)

app.layout = dbc.Container(
    [
        html.H2("🌿 Environmental Sensor Dashboard", className = "text-center my-3"),
        dbc.Row([
            dbc.Col([
                dbc.Button("🔄 Reload Data Now", id = "reload-btn", color = "primary", className = "mb-2 w-100"),
            ],
                    width = 3),
            dbc.Col([
                html.Div([html.Label("🕒 Auto Update Interval", className = "text-end w-100")],
                         className = "d-flex justify-content-end align-items-center h-100"),
            ],
                    width = 2),
            dbc.Col([
                dcc.Dropdown(id = 'interval-dropdown',
                             options = [
                                 {
                                     "label": "Off",
                                     "value": 0
                                 },
                                 {
                                     "label": "1 second",
                                     "value": 1000
                                 },
                                 {
                                     "label": "5 seconds",
                                     "value": 5000
                                 },
                                 {
                                     "label": "10 seconds",
                                     "value": 10000
                                 },
                                 {
                                     "label": "30 seconds",
                                     "value": 30000
                                 },
                             ],
                             value = 0,
                             clearable = False)
            ],
                    width = 3),
            dbc.Col(
                [
                    # Select if you want to jump to the last x-axis element
                    dcc.Checklist(
                        id = 'jump-to-last',
                        options = [{
                            'label': 'Jump to last x-axis element',
                            'value': 'jump'
                        }],
                        value = ['jump'],
                        inputStyle = {"margin-right": "20px"},
                        # inline=True,
                        className = "text-end w-100")
                ],
                width = 3)
        ]),
        html.H4("🌍 Overview", className = "text-left my-4"),
        dcc.Graph(id = 'graph_main'),
        html.H4("🌡️ Temperature", className = "text-left my-4"),
        dcc.Graph(id = 'graph_temp'),
        html.H4("💧 Humidity", className = "text-left my-4"),
        dcc.Graph(id = 'graph_hum'),
        html.H4("📏 Pressure", className = "text-left my-4"),
        dcc.Graph(id = 'graph_pres'),
        html.H4("🧪 Gas Concentrations", className = "text-left my-4"),
        dcc.Graph(id = 'graph_gas'),
        html.H4("🌞 Light Information", className = "text-left my-4"),
        dcc.Graph(id = 'graph_light'),
        dcc.Interval(id = 'auto-update-interval', interval = 1000, n_intervals = 0),  # initially off
        dcc.Store(id = 'trace-visibility', data = {}),
        dcc.Store(id = 'axis-ranges', data = {}),
    ],
    fluid = True)


# Callback: auto update interval
@app.callback(Output('auto-update-interval', 'interval'), Output('auto-update-interval', 'disabled'),
              Input('interval-dropdown', 'value'))
def update_interval(val):
    if val == 0:
        return 1000, True
    else:
        return val, False


@app.callback(Output('graph_main', 'figure'), Output('graph_temp', 'figure'), Output('graph_pres', 'figure'),
              Output('graph_hum', 'figure'), Output('graph_gas', 'figure'), Output('graph_light', 'figure'),
              Output('axis-ranges', 'data'), Output('trace-visibility', 'data'), Input('reload-btn', 'n_clicks'),
              Input('auto-update-interval', 'n_intervals'), Input('graph_main', 'relayoutData'),
              Input('graph_main', 'restyleData'), State('axis-ranges', 'data'), State('trace-visibility', 'data'),
              State('jump-to-last', 'value'))
def update_graph(n_clicks, n_intervals, relayout_data, restyle_data, stored_ranges, stored_visibility, jump_to_last):
    df = dataloader.load_data()

    # === Handle Visibility Logic ===
    # Update the stored visibility states
    update_visibility = stored_visibility.copy() if stored_visibility else {6: 'legendonly'}

    if restyle_data:
        if isinstance(restyle_data[0], dict) and 'visible' in restyle_data[0]:
            changed_visibility = restyle_data[0]['visible'][0]
            affected_traces = restyle_data[1]

            for trace_index in affected_traces:
                update_visibility[str(trace_index)] = changed_visibility

    # === Handle Zoom Logic ===
    # Update the stored zoom ranges
    updated_ranges = stored_ranges.copy() if stored_ranges else {}

    if relayout_data:
        is_reset = ("xaxis.autorange" in relayout_data or "autosize" in relayout_data
                    or any(k.endswith(".autorange") for k in relayout_data))

        if is_reset:
            updated_ranges = {}
        else:
            for key, value in relayout_data.items():
                if (key.startswith("xaxis.range") or (key.startswith("yaxis") and ".range" in key)):
                    updated_ranges[key] = value

    # === Plot Data ===
    trace_defs = [
        ("Temp (°C)", df['SCD41_Temperature'], 'y1'),
        ("Humidity (%RH)", df['SCD41_Humidity'], 'y2'),
        ("CO2 (ppm)", df['SCD41_CO2'], 'y3'),
        ("VOC (Raw)", df['SGP41_VOC'], 'y4'),
        ("NOx (Raw)", df['SGP41_NOX'], 'y5'),
        ("Lux (lx)", df['BH1730FVC_Lux'], 'y6'),
        ("Pressure (hPa)", df['ILPS28QSW_Pressure'], 'y7'),
    ]

    fig1 = go.Figure()
    axis_visibility = {}  # Dictionary to track which y-axes should be shown

    for i, (label, values, yaxis) in enumerate(trace_defs):
        trace_id = str(i)
        visibility = update_visibility.get(trace_id, True)

        # Add trace
        fig1.add_trace(
            go.Scatter(x = df['Timestamp'],
                       y = values,
                       mode = 'lines+markers',
                       name = label,
                       yaxis = yaxis,
                       visible = visibility))

        # Determine corresponding yaxis layout key
        layout_yaxis_key = "yaxis" if yaxis == "y" else f"yaxis{yaxis[-1]}"
        axis_visibility[layout_yaxis_key] = visibility != 'legendonly'

    # Axis definitions
    axis_layouts = {
        "yaxis1":
            dict(title = 'Temperature (°C)',
                 anchor = 'free',
                 side = 'left',
                 tickmode = "sync",
                 rangemode = "nonnegative",
                 constraintoward = "bottom",
                 tickformat = '.1f'),
        "yaxis2":
            dict(title = 'Humidity (%RH)',
                 anchor = 'free',
                 overlaying = 'y1',
                 side = 'left',
                 tickmode = "sync",
                 rangemode = "nonnegative",
                 constraintoward = "bottom",
                 tickformat = '.1f'),
        "yaxis3":
            dict(title = 'CO2 (ppm)',
                 overlaying = 'y1',
                 anchor = 'free',
                 side = 'right',
                 rangemode = "nonnegative",
                 tickmode = "sync",
                 constraintoward = "bottom",
                 tickformat = '.0f'),
        "yaxis4":
            dict(title = 'VOC (Raw)',
                 anchor = 'free',
                 overlaying = 'y1',
                 side = 'right',
                 tickmode = "sync",
                 rangemode = "nonnegative",
                 constraintoward = "bottom",
                 tickformat = '.2s'),
        "yaxis5":
            dict(title = 'NOx (Raw)',
                 anchor = 'free',
                 overlaying = 'y1',
                 side = 'right',
                 tickmode = "sync",
                 rangemode = "nonnegative",
                 constraintoward = "bottom",
                 tickformat = '.2s'),
        "yaxis6":
            dict(title = 'Lux (lx)',
                 anchor = 'free',
                 overlaying = 'y1',
                 side = 'right',
                 tickmode = "sync",
                 rangemode = "nonnegative",
                 constraintoward = "bottom",
                 tickformat = '.0f'),
        "yaxis7":
            dict(title = 'Pressure (hPa)',
                 anchor = 'free',
                 overlaying = 'y1',
                 side = 'right',
                 tickmode = "sync",
                 rangemode = "nonnegative",
                 constraintoward = "bottom",
                 tickformat = '.2f'),
    }

    # Update axis visibility
    position_left = 0
    position_right = 1
    for axis, props in axis_layouts.items():
        props.update(visible = axis_visibility.get(axis, True))

        # If right side axis is not visible, set overlaying to None
        if props.get('side') == 'left' and props['visible']:
            props['position'] = position_left
            position_left += 0.05
        elif props.get('side') == 'right' and props['visible']:
            props['position'] = position_right
            position_right -= 0.05
        fig1.update_layout({axis: props})

    # General layout
    fig1.update_layout(
        template = 'plotly_white',
        legend = dict(orientation = 'h', yanchor = 'bottom', y = 1.02, xanchor = 'center', x = 0.5),
        height = 750,
        margin = dict(t = 0, b = 0),
        xaxis = dict(
            title = 'Timestamp',
            domain = [position_left - 0.05, position_right + 0.05],
        ),
    )

    # === Apply Zoom Ranges ===
    # Preserve zoom levels from stored ranges
    updates = {}

    x_range = updated_ranges.get("xaxis.range[0]"), updated_ranges.get("xaxis.range[1]")
    if all(x_range):
        if jump_to_last:
            x_range_span = pd.to_datetime(x_range[1]) - pd.to_datetime(x_range[0])
            x_range = (df['Timestamp'].iloc[-1] - x_range_span, df['Timestamp'].iloc[-1])
        updates["xaxis"] = dict(range = x_range)

    for i in range(1, 6):
        y_key = f"yaxis{i}" if i > 1 else "yaxis"
        y_range = updated_ranges.get(f"{y_key}.range[0]"), updated_ranges.get(f"{y_key}.range[1]")

        if all(y_range):
            updates[y_key] = dict(range = y_range)

    if updates:
        fig1.update_layout(**updates)

    # Temperature plot
    fig_temp = go.Figure()
    fig_temp.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['SCD41_Temperature'],
                   mode = 'lines+markers',
                   name = "SCD41 Temp",
                   xaxis = "x",
                   yaxis = "y"))
    fig_temp.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['ILPS28QSW_Temperature'],
                   mode = 'lines+markers',
                   name = "ILPS28QSW Temp",
                   xaxis = "x",
                   yaxis = "y"))
    fig_temp.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['BME688_Temperature'],
                   mode = 'lines+markers',
                   name = "BME688 Temp",
                   xaxis = "x",
                   yaxis = "y"))
    fig_temp.update_layout(
        height = 500,
        legend = dict(orientation = 'h', yanchor = 'bottom', y = 1.02, xanchor = 'center', x = 0.5),
        template = 'plotly_white',
        margin = dict(t = 0, b = 0, l = 50, r = 50),
        xaxis = dict(anchor = 'y',),
        yaxis = dict(
            side = "left",
            anchor = "x",
            position = 0,
        ),
    ),

    # Humidity plot
    fig_hum = go.Figure()
    fig_hum.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['SCD41_Humidity'],
                   mode = 'lines+markers',
                   name = "SCD41 Humidity",
                   xaxis = "x",
                   yaxis = "y"))
    fig_hum.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['BME688_Humidity'],
                   mode = 'lines+markers',
                   name = "BME688 Humidity",
                   xaxis = "x",
                   yaxis = "y"))
    fig_hum.update_layout(
        height = 500,
        legend = dict(orientation = 'h', yanchor = 'bottom', y = 1.02, xanchor = 'center', x = 0.5),
        template = 'plotly_white',
        margin = dict(t = 0, b = 0, l = 50, r = 50),
        xaxis = dict(anchor = 'y',),
        yaxis = dict(
            side = "left",
            anchor = "x",
            position = 0,
        ),
    ),

    # # Pressure plot
    fig_pres = go.Figure()
    fig_pres.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['ILPS28QSW_Pressure'],
                   mode = 'lines+markers',
                   name = "ILPS28QSW Pressure",
                   xaxis = "x",
                   yaxis = "y"))
    fig_pres.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['BME688_Pressure'],
                   mode = 'lines+markers',
                   name = "BME688 Pressure",
                   xaxis = "x",
                   yaxis = "y"))
    fig_pres.update_layout(
        height = 500,
        legend = dict(orientation = 'h', yanchor = 'bottom', y = 1.02, xanchor = 'center', x = 0.5),
        template = 'plotly_white',
        margin = dict(t = 0, b = 0, l = 50, r = 50),
        xaxis = dict(anchor = 'y',),
        yaxis = dict(
            side = "left",
            anchor = "x",
            position = 0,
        ),
    ),

    # Gas Concentrations plot
    fig_gas = go.Figure()
    fig_gas.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['SCD41_CO2'],
                   mode = 'lines+markers',
                   name = "SCD41 CO2",
                   xaxis = "x",
                   yaxis = "y"))
    fig_gas.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['SGP41_VOC'],
                   mode = 'lines+markers',
                   name = "SGP41 VOC",
                   xaxis = "x",
                   yaxis = "y2"))
    fig_gas.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['SGP41_NOX'],
                   mode = 'lines+markers',
                   name = "SGP41 NOX",
                   xaxis = "x",
                   yaxis = "y3"))
    fig_gas.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['BME688_Gas_Resistance'],
                   mode = 'lines+markers',
                   name = "BME688 Gas Resistance",
                   xaxis = "x",
                   yaxis = "y4"))
    fig_gas.update_layout(
        height = 500,
        legend = dict(orientation = 'h', yanchor = 'bottom', y = 1.02, xanchor = 'center', x = 0.5),
        template = 'plotly_white',
        margin = dict(t = 0, b = 0, l = 50, r = 50),
        xaxis = dict(
            anchor = 'y',
            domain = [0, 0.9],
        ),
        yaxis = dict(
            side = "left",
            anchor = "x",
            position = 0,
        ),
        yaxis2 = dict(
            side = "right",
            overlaying = 'y',
            anchor = "x",
        ),
        yaxis3 = dict(
            side = "right",
            overlaying = 'y',
            anchor = "free",
            position = 0.95,
        ),
        yaxis4 = dict(side = "right", overlaying = 'y', anchor = "free", position = 1, tickformat = '.2s'),
    ),

    # Light Information plot
    fig_light = go.Figure()
    fig_light.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['BH1730FVC_Visible'],
                   mode = 'lines+markers',
                   name = "BH1730FVC Visible",
                   xaxis = "x",
                   yaxis = "y"))
    fig_light.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['BH1730FVC_IR'],
                   mode = 'lines+markers',
                   name = "BH1730FVC IR",
                   xaxis = "x",
                   yaxis = "y"))
    fig_light.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['BH1730FVC_Lux'],
                   mode = 'lines+markers',
                   name = "BH1730FVC Lux",
                   xaxis = "x",
                   yaxis = "y2"))
    fig_light.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['AS7331_UVA'],
                   mode = 'lines+markers',
                   name = "AS7331 UVA",
                   xaxis = "x",
                   yaxis = "y3"))
    fig_light.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['AS7331_UVB'],
                   mode = 'lines+markers',
                   name = "AS7331 UVB",
                   xaxis = "x",
                   yaxis = "y3"))
    fig_light.add_trace(
        go.Scatter(x = df['Timestamp'],
                   y = df['AS7331_UVC'],
                   mode = 'lines+markers',
                   name = "AS7331 UVC",
                   xaxis = "x",
                   yaxis = "y3"))
    fig_light.update_layout(
        height = 500,
        legend = dict(orientation = 'h', yanchor = 'bottom', y = 1.02, xanchor = 'center', x = 0.5),
        template = 'plotly_white',
        margin = dict(t = 0, b = 0, l = 50, r = 50),
        xaxis = dict(
            anchor = 'y',
            domain = [0, 0.95],
        ),
        yaxis = dict(
            side = "left",
            anchor = "x",
            position = 0,
        ),
        yaxis2 = dict(
            side = "right",
            overlaying = 'y',
            anchor = "x",
        ),
        yaxis3 = dict(
            side = "right",
            overlaying = 'y',
            anchor = "free",
            position = 1,
        ),
    ),

    # Update x axis range on all subplots
    fig_temp.update_xaxes(range = x_range)
    fig_hum.update_xaxes(range = x_range)
    fig_pres.update_xaxes(range = x_range)
    fig_gas.update_xaxes(range = x_range)
    fig_light.update_xaxes(range = x_range)

    return fig1, fig_temp, fig_pres, fig_hum, fig_gas, fig_light, updated_ranges, update_visibility


if __name__ == '__main__':
    app.run(debug = True)
