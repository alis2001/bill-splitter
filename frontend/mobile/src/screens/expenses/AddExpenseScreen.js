import React, { useState, useEffect, useContext } from 'react';
import { 
  View, Text, StyleSheet, ScrollView, Alert, TouchableOpacity,
  Animated, Keyboard 
} from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { TextInput, Button, SegmentedButtons } from 'react-native-paper';
import { Ionicons } from '@expo/vector-icons';
import { AuthContext } from '../../context/AuthContext';
import { eventsAPI } from '../../services/apiService';

const SPLIT_TYPES = [
  { value: 'equal', label: 'Equal Split', icon: 'people' },
  { value: 'percentage', label: 'By %', icon: 'pie-chart' },
  { value: 'custom', label: 'Custom', icon: 'calculator' }
];

const AddExpenseScreen = ({ navigation, route }) => {
  const { event } = route.params;
  const { user } = useContext(AuthContext);
  const [description, setDescription] = useState('');
  const [amount, setAmount] = useState('');
  const [participants, setParticipants] = useState([]);
  const [selectedPayer, setSelectedPayer] = useState(user?.id);
  const [splitType, setSplitType] = useState('equal');
  const [loading, setLoading] = useState(false);
  const slideAnim = new Animated.Value(300);

  useEffect(() => {
    loadParticipants();
    Animated.spring(slideAnim, {
      toValue: 0,
      tension: 50,
      friction: 8,
      useNativeDriver: true,
    }).start();
  }, []);

  const loadParticipants = async () => {
    const result = await eventsAPI.getParticipants(event.id);
    if (result.success) {
      const allParticipants = [
        { user_id: user?.id, user: { name: user?.name, family_name: user?.family_name } },
        ...(result.data.participants || [])
      ];
      setParticipants(allParticipants);
    }
  };

  const handleAddExpense = async () => {
    if (!description.trim()) {
      Alert.alert('Missing Info', 'What was this expense for?');
      return;
    }

    if (!amount || parseFloat(amount) <= 0) {
      Alert.alert('Invalid Amount', 'Please enter a valid amount');
      return;
    }

    Keyboard.dismiss();
    setLoading(true);

    const result = await eventsAPI.addExpense(event.id, {
      description: description.trim(),
      amount: parseFloat(amount),
      payer_id: selectedPayer,
      split_type: splitType
    });

    setLoading(false);

    if (result.success) {
      navigation.goBack();
    } else {
      Alert.alert('Error', result.error);
    }
  };

  const renderPayerSelector = () => (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>Who paid?</Text>
      {participants.map((participant) => (
        <TouchableOpacity
          key={participant.user_id}
          style={[
            styles.participantCard,
            selectedPayer === participant.user_id && styles.selectedParticipant
          ]}
          onPress={() => setSelectedPayer(participant.user_id)}
          activeOpacity={0.7}
        >
          <View style={styles.participantInfo}>
            <View style={[
              styles.avatar,
              selectedPayer === participant.user_id && styles.selectedAvatar
            ]}>
              <Text style={[
                styles.avatarText,
                selectedPayer === participant.user_id && styles.selectedAvatarText
              ]}>
                {participant.user.name?.charAt(0)}
              </Text>
            </View>
            <Text style={[
              styles.participantName,
              selectedPayer === participant.user_id && styles.selectedParticipantName
            ]}>
              {participant.user_id === user?.id ? 'You' : 
                `${participant.user.name} ${participant.user.family_name}`}
            </Text>
          </View>
          {selectedPayer === participant.user_id && (
            <Ionicons name="checkmark-circle" size={24} color="#38A169" />
          )}
        </TouchableOpacity>
      ))}
    </View>
  );

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <TouchableOpacity onPress={() => navigation.goBack()}>
          <Ionicons name="close" size={24} color="#2D3748" />
        </TouchableOpacity>
        <Text style={styles.title}>Add Expense</Text>
        <View style={{ width: 24 }} />
      </View>

      <Animated.View style={[styles.content, { transform: [{ translateY: slideAnim }] }]}>
        <ScrollView showsVerticalScrollIndicator={false}>
          <View style={styles.inputSection}>
            <TextInput
              label="What was this for?"
              value={description}
              onChangeText={setDescription}
              mode="outlined"
              style={styles.input}
              placeholder="e.g., Dinner, Gas, Hotel room"
              autoFocus
            />

            <View style={styles.amountInput}>
              <TextInput
                label="Amount"
                value={amount}
                onChangeText={setAmount}
                mode="outlined"
                keyboardType="numeric"
                style={styles.input}
                placeholder="0.00"
                left={<TextInput.Icon icon="currency-usd" />}
              />
              <TouchableOpacity 
                style={styles.splitPreview}
                disabled={!amount || participants.length === 0}
              >
                <Text style={styles.splitText}>
                  {amount && participants.length > 0 
                    ? `$${(parseFloat(amount) / participants.length).toFixed(2)} each`
                    : 'Split preview'
                  }
                </Text>
              </TouchableOpacity>
            </View>
          </View>

          {renderPayerSelector()}

          <View style={styles.section}>
            <Text style={styles.sectionTitle}>How to split?</Text>
            <SegmentedButtons
              value={splitType}
              onValueChange={setSplitType}
              buttons={SPLIT_TYPES.map(type => ({
                value: type.value,
                label: type.label,
                icon: type.icon
              }))}
              style={styles.segmentedButtons}
            />
          </View>
        </ScrollView>

        <View style={styles.footer}>
          <Button
            mode="contained"
            onPress={handleAddExpense}
            loading={loading}
            disabled={loading || !description.trim() || !amount}
            style={[
              styles.addButton,
              (!description.trim() || !amount) && styles.disabledButton
            ]}
            contentStyle={styles.buttonContent}
          >
            {loading ? 'Adding...' : 'Add Expense'}
          </Button>
        </View>
      </Animated.View>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#FAFAFA',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#FFFFFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E2E8F0',
    elevation: 2,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.1,
    shadowRadius: 2,
  },
  title: {
    fontSize: 18,
    fontWeight: '600',
    color: '#2D3748',
  },
  content: {
    flex: 1,
    padding: 16,
  },
  inputSection: {
    marginBottom: 24,
  },
  input: {
    marginBottom: 16,
    backgroundColor: '#FFFFFF',
  },
  amountInput: {
    position: 'relative',
  },
  splitPreview: {
    position: 'absolute',
    right: 12,
    top: 28,
    backgroundColor: '#F7FAFC',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 6,
  },
  splitText: {
    fontSize: 12,
    color: '#718096',
    fontWeight: '500',
  },
  section: {
    marginBottom: 24,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    marginBottom: 12,
    color: '#2D3748',
  },
  participantCard: {
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    elevation: 1,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
  },
  selectedParticipant: {
    backgroundColor: '#E6FFFA',
    borderWidth: 2,
    borderColor: '#38A169',
  },
  participantInfo: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  avatar: {
    width: 40,
    height: 40,
    borderRadius: 20,
    backgroundColor: '#E2E8F0',
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  selectedAvatar: {
    backgroundColor: '#38A169',
  },
  avatarText: {
    fontSize: 16,
    fontWeight: '600',
    color: '#2D3748',
  },
  selectedAvatarText: {
    color: '#FFFFFF',
  },
  participantName: {
    fontSize: 16,
    fontWeight: '500',
    color: '#2D3748',
  },
  selectedParticipantName: {
    color: '#38A169',
    fontWeight: '600',
  },
  segmentedButtons: {
    backgroundColor: '#FFFFFF',
  },
  footer: {
    paddingTop: 16,
    paddingBottom: 8,
  },
  addButton: {
    backgroundColor: '#1A365D',
  },
  disabledButton: {
    backgroundColor: '#A0AEC0',
  },
  buttonContent: {
    paddingVertical: 8,
  },
});

export default AddExpenseScreen;