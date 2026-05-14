#same code written as a python file 
#exactly similar code is written with brief explanations in another file that I have submitted '24K-0826-MLcomp.ipynb' which is a jupyter notebook file this has this entire code broken down into cells with breif explanations
import numpy as np
import pandas as pd
import time

from sklearn.pipeline import Pipeline
from sklearn.preprocessing import OrdinalEncoder, LabelEncoder
from sklearn.compose import ColumnTransformer
from sklearn.model_selection import StratifiedKFold
from sklearn.metrics import balanced_accuracy_score, confusion_matrix
from sklearn.tree import DecisionTreeClassifier
from sklearn.naive_bayes import GaussianNB
from sklearn.linear_model import RidgeClassifier
from sklearn.cluster import KMeans
from sklearn.base import BaseEstimator, ClassifierMixin


RNG_SEED = 42

print("Loading data from disk.")

train_raw = pd.read_csv("train.csv")
test_raw  = pd.read_csv("test.csv")

test_ids = test_raw["id"].values

print(f"Train shape : {train_raw.shape}")
print(f"Test shape  : {test_raw.shape}")
print(f"Class distribution:\n{train_raw['Irrigation_Need'].value_counts()}\n")

DROP_COLS    = ["id", "Irrigation_Need"]
TARGET_COL   = "Irrigation_Need"


categorical_cols = [
    "Soil_Type", "Crop_Type", "Crop_Growth_Stage",
    "Season", "Irrigation_Type", "Water_Source",
    "Mulching_Used", "Region"
]
numerical_cols = [
    "Soil_pH", "Soil_Moisture", "Organic_Carbon",
    "Electrical_Conductivity", "Temperature_C", "Humidity",
    "Rainfall_mm", "Sunlight_Hours", "Wind_Speed_kmh",
    "Field_Area_hectare", "Previous_Irrigation_mm"
]

feature_cols   = categorical_cols + numerical_cols
feature_matrix = train_raw[feature_cols].copy()
target_labels  = train_raw[TARGET_COL].copy()
test_features  = test_raw[feature_cols].copy()


label_enc        = LabelEncoder()
target_encoded   = label_enc.fit_transform(target_labels)

print(f"Encoded classes : {list(label_enc.classes_)}")
print(f"Label mapping   : {dict(zip(label_enc.classes_, label_enc.transform(label_enc.classes_)))}\n")


categorical_transformer = OrdinalEncoder(
    handle_unknown="use_encoded_value",
    unknown_value=-1
)



soil_crop_pipeline = ColumnTransformer(
    transformers=[
        ("cat_ord", categorical_transformer, categorical_cols),
        ("num_pass", "passthrough",            numerical_cols),
    ],
    remainder="drop",
    n_jobs=1
)

class KMeansClassifierWrapper(BaseEstimator, ClassifierMixin):
    def __init__(self, n_clusters=3, random_state=RNG_SEED, n_init=10, max_iter=300):
        self.n_clusters   = n_clusters
        self.random_state = random_state
        self.n_init       = n_init
        self.max_iter     = max_iter

    def fit(self, feature_block, label_block):
       

        self._km = KMeans(
            n_clusters=self.n_clusters,
            random_state=self.random_state,
            n_init=self.n_init,
            max_iter=self.max_iter
        )
        cluster_ids = self._km.fit_predict(feature_block)

        self._cluster_label_map = {}
        for cluster_id in range(self.n_clusters):
            mask = cluster_ids == cluster_id
            if mask.sum() == 0:
                self._cluster_label_map[cluster_id] = 0
            else:
                counts = np.bincount(label_block[mask].astype(int),
                                     minlength=len(np.unique(label_block)))
                self._cluster_label_map[cluster_id] = int(np.argmax(counts))

        self.classes_ = np.unique(label_block)
        return self

    def predict(self, feature_block):
        cluster_ids = self._km.predict(feature_block)
        return np.array([self._cluster_label_map[c] for c in cluster_ids])
    
def run_stratified_cv(model_pipeline, feature_matrix, target_encoded, n_splits=5, label="Model"):
    skf         = StratifiedKFold(n_splits=n_splits, shuffle=True, random_state=RNG_SEED)
    fold_scores = []
    last_cm     = None
    t0          = time.perf_counter()

    for fold_idx, (train_idx, val_idx) in enumerate(skf.split(feature_matrix, target_encoded), start=1):
        fold_train_features = feature_matrix.iloc[train_idx]
        fold_val_features   = feature_matrix.iloc[val_idx]
        fold_train_labels   = target_encoded[train_idx]
        fold_val_labels     = target_encoded[val_idx]

        model_pipeline.fit(fold_train_features, fold_train_labels)
        fold_predictions = model_pipeline.predict(fold_val_features)

        fold_ba = balanced_accuracy_score(fold_val_labels, fold_predictions)
        fold_scores.append(fold_ba)
        last_cm = confusion_matrix(fold_val_labels, fold_predictions)

        print(f"    Fold {fold_idx}/5 — balanced_accuracy = {fold_ba:.4f}")

    elapsed      = time.perf_counter() - t0
    mean_bal_acc = float(np.mean(fold_scores))
    std_bal_acc  = float(np.std(fold_scores))

    print(f"  ── {label} CV Summary ──")
    print(f"     Mean Balanced Accuracy : {mean_bal_acc:.4f}")
    print(f"     Total time             : {elapsed:.1f}s")
    print(f"\n  Confusion Matrix (last fold)")
    print(f"  Classes: {list(label_enc.classes_)}")
    print(f"{last_cm}\n")

    return mean_bal_acc, last_cm

print("PHASE 1 — Base Model Benchmarks\n")

# 1. Decision Tree 
tree_model = Pipeline([
    ("preproc",    soil_crop_pipeline),
    ("classifier", DecisionTreeClassifier(max_depth=10, random_state=RNG_SEED))
])
dt_base_score, _ = run_stratified_cv(tree_model, feature_matrix, target_encoded, label="Decision Tree (base)")

# 2. Naive Bayes
nb_pipeline = Pipeline([
    ("preproc",    soil_crop_pipeline),
    ("classifier", GaussianNB())
])
nb_score, _ = run_stratified_cv(nb_pipeline, feature_matrix, target_encoded, label="Gaussian Naive Bayes")

# 3. Ridge Classifier, 
# Using linear regression for classification
# I tried messing with the alpha parameter but it didn't help much

ridge_pipeline = Pipeline([
    ("preproc",    soil_crop_pipeline),
    ("classifier", RidgeClassifier(alpha=1.0, class_weight="balanced"))
])
ridge_score, _ = run_stratified_cv(ridge_pipeline, feature_matrix, target_encoded, label="Ridge Classifier")

# 4. K-Means
kmeans_pipeline = Pipeline([
    ("preproc",    soil_crop_pipeline),
    ("classifier", KMeansClassifierWrapper(n_clusters=3, random_state=RNG_SEED))
])
kmeans_score, _ = run_stratified_cv(kmeans_pipeline, feature_matrix, target_encoded, label="K-Means Wrapper")

print("PHASE 2 — Decision Tree Tuning\n")

# Attempt 1 - Overfitting the tree - FAILED I AM SED....
# The tree just memorizes the training data and fails on anything new.
overfitted_tree = Pipeline([
    ("preproc",    soil_crop_pipeline),
    ("classifier", DecisionTreeClassifier(max_depth=None, random_state=RNG_SEED))
])
overfitted_score, overfitted_cm = run_stratified_cv(overfitted_tree, feature_matrix, target_encoded, label="Overfitted Decision Tree")

# Attempt 2 - Ignored the class imbalance - FAILED I AM SED AGAIN.....
# Without telling it to care about the minority class, it gets lazy and guesses 'Low' too often. IT IS AS LAZY AS ME tch tch...
imbalanced_tree = Pipeline([
    ("preproc",    soil_crop_pipeline),
    ("classifier", DecisionTreeClassifier(max_depth=15, random_state=RNG_SEED))
])
imbalanced_score, imbalanced_cm = run_stratified_cv(imbalanced_tree, feature_matrix, target_encoded, label="Imbalanced Decision Tree")

# Winner Hai: Optimized Tree
# Balanced weights force the tree to treat the 'HIGHH' minority class like it's important.
optimised_tree = Pipeline([
    ("preproc",    soil_crop_pipeline),
    ("classifier", DecisionTreeClassifier(
        max_depth=20,
        class_weight="balanced",
        min_samples_leaf=10,    
        random_state=RNG_SEED
    ))
])
optimised_score, optimised_cm = run_stratified_cv(optimised_tree, feature_matrix, target_encoded, label="Optimised Decision Tree")

print("PHASE 3 — Final Training & Submission\n")

final_pipeline = Pipeline([
    ("preproc",    soil_crop_pipeline),
    ("classifier", DecisionTreeClassifier(
        max_depth=20,
        class_weight="balanced",
        min_samples_leaf=10,
        random_state=RNG_SEED
    ))
])

print("Fitting the final model on all the training data...")
final_pipeline.fit(feature_matrix, target_encoded)

print("Making predictions for test.csv...")
test_predictions_encoded = final_pipeline.predict(test_features)
test_predictions_labels  = label_enc.inverse_transform(test_predictions_encoded)

# Creating the final submission file
submission_df = pd.DataFrame({
    "id":              test_ids,
    "Irrigation_Need": test_predictions_labels
})
submission_path = "final_submission.csv"
submission_df.to_csv(submission_path, index=False)

print(f"Saved: {submission_path}")
print("Ssab kuch Khatam, FINALLY!!!!!")

