.pragma library

function allFields(pageState) {
    const fields = [];
    if (pageState.positionFields) {
        fields.push(...pageState.positionFields);
    }
    if (pageState.dimensionFields) {
        fields.push(...pageState.dimensionFields);
    }
    return fields;
}

function resetForm(pageState) {
    const nextRequestSpec = pageState.requestSpec;
    const nextValues = {
        "modelName": nextRequestSpec ? nextRequestSpec.defaultName : ""
    };
    for (const field of allFields(pageState)) {
        nextValues[field.key] = field.defaultValue;
    }
    pageState.formValues = nextValues;
    pageState.axisValue = nextRequestSpec && nextRequestSpec.defaultAxis ? nextRequestSpec.defaultAxis : "Z";
}

function fieldValue(pageState, fieldKey) {
    if (!pageState.formValues || pageState.formValues[fieldKey] === undefined || pageState.formValues[fieldKey] === null) {
        return "";
    }
    return String(pageState.formValues[fieldKey]);
}

function setFieldValue(pageState, fieldKey, nextValue) {
    const nextValues = Object.assign({}, pageState.formValues || {});
    nextValues[fieldKey] = nextValue;
    pageState.formValues = nextValues;
}

function numericValue(pageState, fieldKey, fallbackValue) {
    const parsedValue = Number(fieldValue(pageState, fieldKey));
    return isFinite(parsedValue) ? parsedValue : fallbackValue;
}

function validateField(pageState, field) {
    const rawValue = fieldValue(pageState, field.key).trim();
    const parsedValue = Number(rawValue);
    if (rawValue.length === 0 || !isFinite(parsedValue)) {
        return {
            "success": false,
            "fieldKey": field.key,
            "message": qsTr("Enter a valid number for %1.").arg(field.label)
        };
    }
    if (field.positiveOnly && parsedValue <= 0) {
        return {
            "success": false,
            "fieldKey": field.key,
            "message": qsTr("%1 must be greater than zero.").arg(field.label)
        };
    }
    return {
        "success": true,
        "value": parsedValue
    };
}

function assignPath(target, path, value) {
    let current = target;
    for (let index = 0; index < path.length - 1; ++index) {
        const segment = path[index];
        if (current[segment] === undefined || current[segment] === null || typeof current[segment] !== "object") {
            current[segment] = {};
        }
        current = current[segment];
    }
    current[path[path.length - 1]] = value;
}

function validateShapeSpecific(pageState, param) {
    if (!pageState.requestSpec) {
        return {
            "success": false,
            "fieldKey": "",
            "message": qsTr("Geometry create request is not configured.")
        };
    }

    if (pageState.requestSpec.shapeType === "torus" && param.minorRadius >= param.majorRadius) {
        return {
            "success": false,
            "fieldKey": "minorRadius",
            "message": qsTr("Minor radius must be smaller than major radius.")
        };
    }

    return {
        "success": true
    };
}

function buildValidatedRequest(pageState) {
    if (!pageState.requestSpec) {
        return {
            "success": false,
            "fieldKey": "",
            "message": qsTr("Geometry create request is not configured.")
        };
    }

    const param = {
        "modelName": fieldValue(pageState, "modelName").trim().length > 0
            ? fieldValue(pageState, "modelName").trim()
            : pageState.requestSpec.defaultName,
        "source": pageState.requestSource
    };

    for (const field of allFields(pageState)) {
        const fieldResult = validateField(pageState, field);
        if (!fieldResult.success) {
            return fieldResult;
        }
        assignPath(param, field.path || [field.key], fieldResult.value);
    }

    if (pageState.supportsAxis) {
        param.axis = pageState.axisValue;
    }

    const shapeValidation = validateShapeSpecific(pageState, param);
    if (!shapeValidation.success) {
        return shapeValidation;
    }

    return {
        "success": true,
        "request": {
            "module": pageState.requestSpec.module,
            "action": pageState.requestSpec.action,
            "param": param
        }
    };
}

function advisoryMessage(pageState) {
    if (!pageState.requestSpec || pageState.requestSpec.shapeType !== "torus") {
        return "";
    }

    const majorRadius = numericValue(pageState, "majorRadius", NaN);
    const minorRadius = numericValue(pageState, "minorRadius", NaN);
    if (!isFinite(majorRadius) || !isFinite(minorRadius)) {
        return "";
    }

    return minorRadius >= majorRadius
        ? qsTr("Minor radius should stay below major radius for a valid torus.")
        : "";
}

function formatNumber(value) {
    return isFinite(value) ? value.toFixed(3) : "0.000";
}

function derivedMetrics(pageState) {
    if (!pageState.requestSpec) {
        return [];
    }

    if (pageState.requestSpec.shapeType === "box") {
        const sizeX = numericValue(pageState, "sizeX", 0.0);
        const sizeY = numericValue(pageState, "sizeY", 0.0);
        const sizeZ = numericValue(pageState, "sizeZ", 0.0);
        const diagonal = Math.sqrt(sizeX * sizeX + sizeY * sizeY + sizeZ * sizeZ);
        return [
            {
                "label": qsTr("Volume"),
                "value": formatNumber(sizeX * sizeY * sizeZ) + " mm³",
                "accent": "accentA"
            },
            {
                "label": qsTr("Diagonal"),
                "value": formatNumber(diagonal) + " mm",
                "accent": "accentE"
            }
        ];
    }

    if (pageState.requestSpec.shapeType === "cylinder") {
        const radius = numericValue(pageState, "radius", 0.0);
        const height = numericValue(pageState, "height", 0.0);
        return [
            {
                "label": qsTr("Volume"),
                "value": formatNumber(Math.PI * radius * radius * height) + " mm³",
                "accent": "accentA"
            },
            {
                "label": qsTr("Surface Area"),
                "value": formatNumber(2 * Math.PI * radius * (radius + height)) + " mm²",
                "accent": "accentC"
            }
        ];
    }

    if (pageState.requestSpec.shapeType === "sphere") {
        const radius = numericValue(pageState, "radius", 0.0);
        return [
            {
                "label": qsTr("Volume"),
                "value": formatNumber((4.0 / 3.0) * Math.PI * Math.pow(radius, 3)) + " mm³",
                "accent": "accentA"
            },
            {
                "label": qsTr("Surface Area"),
                "value": formatNumber(4.0 * Math.PI * radius * radius) + " mm²",
                "accent": "accentC"
            },
            {
                "label": qsTr("Diameter"),
                "value": formatNumber(radius * 2.0) + " mm",
                "accent": "accentE"
            }
        ];
    }

    const majorRadius = numericValue(pageState, "majorRadius", 0.0);
    const minorRadius = numericValue(pageState, "minorRadius", 0.0);
    return [
        {
            "label": qsTr("Volume"),
            "value": formatNumber(2 * Math.PI * Math.PI * majorRadius * minorRadius * minorRadius) + " mm³",
            "accent": "accentA"
        },
        {
            "label": qsTr("Surface Area"),
            "value": formatNumber(4 * Math.PI * Math.PI * majorRadius * minorRadius) + " mm²",
            "accent": "accentC"
        },
        {
            "label": qsTr("Outer Diameter"),
            "value": formatNumber((majorRadius + minorRadius) * 2.0) + " mm",
            "accent": "accentE"
        }
    ];
}
